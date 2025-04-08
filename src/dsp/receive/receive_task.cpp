//
// Created by Angel Dust on 04/04/2025.
//

#include "receive_task.h"
#include "dsp/dsp_buffers.h"
#include "dsp/firFilter.h"
#include "dsp/dsp_common.h"
#include "radio.h"
#include "status.h"
#include "hw/stm32f4xx/timers.h"
#include "config.h"
#include "FIFO.h"
#include "types.h"
#include "ui/view.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include "dsp/decimation/dsp_decimators.h"
#include <sys/_stdint.h>

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

        uint16_t block_size_in = status.block_size_bytes / 2;
        uint16_t block_size_out = status.decimated_block_size;

        uint32_t free = output_stream.free(&out_p);
        uint16_t av = input_stream.available(&in_p);

        if (av >= DSP_FIFO_BLOCK_BYTES && free >= (DSP_FIFO_BLOCK_BYTES / status.decimation_factor)) {

            this->status.processed_blocks++;
            av = DSP_FIFO_BLOCK_BYTES;

            in_start = in_p;

            // Wrap the complex_t buffers with an adc_type buffer
            buffer_t<adc_type> buff_tmp = {(adc_type *)dsp_temp_buf.p, 16};

            // Process input block
            while (av >= status.block_size_bytes) {

                buffer_t<adc_type> buff = {(adc_type *)in_p, block_size_in};
                buffer_t<adc_type> buff_out = {(adc_type *)out_p, block_size_out};

                decimator_i_0.decimate(buff, buff_tmp, 0, status.n_channels);
                if (status.decimation_factor > 2) {
                    decimator_i_1.decimate(buff_tmp, buff_out, 0, status.n_channels);
                }

                out_p += status.decimated_block_size_bytes;
                in_p += status.block_size_bytes;
                av -= status.block_size_bytes;
            }

            uint32_t processed = DSP_FIFO_BLOCK_BYTES - av;

            FIFO_ERROR fifo_res = input_stream.consume(processed, &in_start);

            if (fifo_res != FIFO_ERROR_NONE) {
                status.fifo_underruns++;
            }

            fifo_res = output_stream.feed(processed / status.decimation_factor);

            if (fifo_res != FIFO_ERROR_NONE) {
                status.fifo_overruns++;
            }

        } else {
            if (free < DSP_FIFO_BLOCK_BYTES) {
                status.fifo_overruns++;
            } else {
                status.fifo_underruns++;
            }
        }
    }
}

void ReceiveTask::start() {

    dsp_set_real_time(true);

    int dec_factor = 1;
    status.sample_rate = config.fft.sample_rate;
    // calculate decimation ratio to get to audio bandwidth
    while (status.sample_rate > audio_bw_hz * 2) {
        dec_factor <<= 1;
        status.sample_rate /= 2;
    }

    // If the decimation factor is greater than

    status.direction = DSP_DIRECTION_IN;
    status.bandwidth = radio::get_bandwidth_hz(); // This is the desired filter bandwidth based on current modulation and user selected filter
    status.decimation_factor = dec_factor;
    status.bits_per_sample = 16;
    status.n_channels = 2;
    status.block_size_bytes = dsp_temp_buf.size_bytes;
    status.decimated_block_size = dsp_temp_buf.count / dec_factor / (status.n_channels == 1 ? 2 : 1);
    status.decimated_block_size_bytes = status.block_size_bytes / dec_factor / (status.n_channels == 1 ? 2 : 1);

    // decimator_i_0.config(config.fft.sample_rate, status.bandwidth, dec_factor, status.bandwidth - 1000);
    uint8_t factor = 2;
    uint32_t stage_1_fs = (config.fft.sample_rate / factor) - 1;

    decimator_i_0.config(config.fft.sample_rate, stage_1_fs, factor);
    if (dec_factor > 2) {
        factor = dec_factor - factor;
        decimator_i_1.config(stage_1_fs, status.bandwidth, factor);
    }
    // Start task processing timer
    // TODO: This should be done by the caller of this method and be generic for all tasks
    HAL_TIM_Base_Start_IT(&TASKS_TIMER_HANDLE);

    // Se the fifo processing frequency
    update_timer(TASKS_TIMER_TYPEDEF, 2, TASKS_TIMER_TYPEDEF_CLOCK_HZ / 10000);

    bool ret = radio_config({.direction = RF_DIRECTION_RX,
                             .sample_freq = status.sample_rate,
                             .freq = 0,
                             .mode = DSP}); // Radio mode is DSP so the signal is routed to the audio amp

    status.status = DSP_STATUS_RUNNING;

    if (!ret) {
        halt(DSP_ERR);
    }
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
