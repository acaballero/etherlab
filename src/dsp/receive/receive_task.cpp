//
// Created by Angel Dust on 04/04/2025.
//

#include "receive_task.h"
#include "arm_math.h"
#include "dsp/blocks/dc_block.h"
#include "dsp/dsp_buffers.h"
#include "dsp/fft/fft.h"
#include "dsp/fft/fft_types.h"
#include "dsp/firFilter.h"
#include "dsp/dsp_common.h"
#include "dsp/modulation/dsp_demodulate.hpp"
#include "main_board.h"
#include "radio.h"
#include "status.h"
#include "hw/stm32f4xx/timers.h"
#include "config.h"
#include "FIFO.h"
#include "stm32f4xx_hal.h"
#include "types.h"
#include "ui/view.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include "dsp/decimation/dsp_decimators.h"
#include <cstddef>
#include <memory>
#include "printf.h"
#include "utils.hpp"

/* Should be defined in the HW abstraction layer */
extern TIM_HandleTypeDef TASKS_TIMER_HANDLE;

ReceiveTask::ReceiveTask(void (*onSucess)(), void (*onError)(DSP_ERROR)) {
    on_error = onError;
    on_success = onSucess;
}

void ReceiveTask::work() {
    if (status.status == DSP_STATUS_RUNNING) {

        char *in_start;
        char *in_p;
        char *out_p;

        uint32_t free = output_stream.free(&out_p);
        uint32_t av = input_stream.available(&in_p);

        if (av >= DSP_FIFO_BLOCK_BYTES && free >= (DSP_FIFO_BLOCK_BYTES / status.decimation_factor)) {

            this->status.processed_blocks++;
            av = DSP_FIFO_BLOCK_BYTES;

            in_start = in_p;

            // Process input block
            while (av >= status.block_size_bytes) {

                uint16_t block_size_in = status.block_size_bytes / sizeof(complex_t_f32);
                uint16_t block_size_out = block_size_in >> 1;

                buffer_t<float32_t> buff_out_f32 = {(float32_t *)out_p, block_size_out};
                buffer_t<adc_type> buff_out_s16 = {(adc_type *)out_p, block_size_out};
                buffer_t<complex_t_f32> buff_out = {(complex_t_f32 *)out_p, block_size_out};

                for (int i = 0; i < n_decimators; i++) {

                    buffer_t<complex_t_f32> buff = {(complex_t_f32 *)in_p, block_size_in};
                    buffer_t<complex_t_f32> buff_out = {(complex_t_f32 *)out_p, block_size_out};

                    if (i < n_decimators - 1) {
                        // Half-band decimators
                        if (i == 0) {
                            decimators_0[i].decimate(buff, buff);
                        } else {
                            decimators_0[i].decimate(buff, buff);
                        }

                    } else {
                        buffer_t<adc_type> buff_tmp = {(adc_type *)dsp_temp_buf.p, block_size_in};
                        // Signal decimators
                        decimators_1[0].decimate(buff, buff_out);
                    }

                    block_size_in >>= 1;
                    block_size_out >>= 1;
                }

                // DC block;
                block_i.filter(buff_out_f32, 2, 0);
                block_q.filter(buff_out_f32, 2, 1);

                demodulator->work(buff_out, buff_out_s16);

                out_p += status.decimated_block_size_bytes;
                in_p += status.block_size_bytes;
                av -= status.block_size_bytes;
            }

            uint32_t processed = DSP_FIFO_BLOCK_BYTES - av;

            input_stream.consume(processed, &in_start);
            output_stream.feed(processed / status.decimation_factor);

        } else {
            if (free < DSP_FIFO_BLOCK_BYTES) {
                status.fifo_overruns++;
            } else {
                status.fifo_underruns++;
            }
        }
    }
}

std::unique_ptr<dsp::demodulator> ReceiveTask::get_modulator() {

    switch (main_board::getModulationMode()) {
        case AM:
            return std::make_unique<dsp::am_demodulator>();
        case CW:
        case SSB_LSB:
        case SSB_USB:
            return std::make_unique<dsp::ssb_demodulator>();
        case FM:
            return std::make_unique<dsp::fm_demodulator>();
        default:
            return std::make_unique<dsp::ssb_demodulator>();
    }
}

bool ReceiveTask::start() {

    dsp_set_real_time(true);

    // bool b = fft_config(audio_bw_hz * 2);

    // if (!b) {
    //     this->halt(DSP_ERR);
    //     return;
    // }

    int dec_factor = 1;
    status.sample_rate = config.fft.sample_rate;
    // calculate decimation ratio to get to audio bandwidth
    while (status.sample_rate > audio_bw_hz * 2 && dec_factor < config.fft.max_decimation_factor) {
        dec_factor <<= 1;
        status.sample_rate /= 2;
    }

    // If the decimation factor is greater than

    status.direction = DSP_DIRECTION_IN;

    MODULATION_MODE mod = main_board::getModulationMode();

    if (mod == SSB_LSB || mod == SSB_USB) {
        status.bandwidth = radio::get_bandwidth_hz();
    } else {
        status.bandwidth = radio::get_bandwidth_hz() / 2; // Desired filter bandwidth based on current modulation and user selected filter
    }
    status.decimation_factor = dec_factor;
    status.bits_per_sample = 16;
    status.n_channels = 2;
    status.block_size_bytes = dsp_temp_buf.size_bytes;
    status.decimated_block_size = dsp_temp_buf.count / dec_factor / (status.n_channels == 1 ? 2 : 1);
    status.decimated_block_size_bytes = status.block_size_bytes / dec_factor / (status.n_channels == 1 ? 2 : 1);

    // First staes are half-band filters (https://en.wikipedia.org/wiki/Half-band_filter)
    uint8_t factor = 2;
    uint8_t dec = dec_factor;

    uint32_t stage_fs = config.fft.sample_rate;
    uint32_t next_stage_fs = (config.fft.sample_rate / 4);
    bool ret = decimators_0[0].config(stage_fs, next_stage_fs, factor);
    if (!ret) {
        halt(DSP_ERR);
        return false;
    }
    dec = dec / factor;
    n_decimators = 1;

    if (dec > 2) {
        stage_fs = next_stage_fs;
        next_stage_fs = stage_fs / 4;
        ret = decimators_0[1].config(stage_fs, next_stage_fs, factor);
        if (!ret) {
            halt(DSP_ERR);
            return false;
        }
        dec = dec / factor;
        n_decimators++;
    }

    if (dec) {
        stage_fs = next_stage_fs;
        factor = dec;
        ret = decimators_1[0].config(stage_fs, status.bandwidth, factor);
        if (!ret) {
            halt(DSP_ERR);
            return false;
        }
        n_decimators++;
    }
    // Start task processing timer
    // TODO: This should be done by the caller of this method and be generic for all tasks
    HAL_TIM_Base_Start_IT(&TASKS_TIMER_HANDLE);

    // Se the fifo processing frequency
    update_timer(TASKS_TIMER_TYPEDEF, 4, TASKS_TIMER_TYPEDEF_CLOCK_HZ / 10000);

    ret = radio_config({.direction = RF_DIRECTION_RX,
                        .sample_freq = status.sample_rate,
                        .freq = 0,
                        .mode = DSP}); // Radio mode is DSP so the signal is routed to the audio amp

    status.status = DSP_STATUS_RUNNING;

    demodulator = get_modulator();

    if (!ret) {
        halt(DSP_ERR);
        return false;
    }

    return true;
}

void ReceiveTask::stop() {

    if (status.status != DSP_STATUS_STOPPED) {

        status.status = DSP_STATUS_STOPPED;

        dsp_set_real_time(false);

        // Stop task processing timer
        HAL_TIM_Base_Stop_IT(&TASKS_TIMER_HANDLE);

        Task::stop(); // Let the base class finish

        radio_config({RF_DIRECTION_RX, 0});
    }
}
