//
// Created by Angel Dust on 04/04/2025.
//

#include "receive_task_base.h"
#include "arm_math.h"
#include "dsp/blocks/dc_block.h"
#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "dsp/decimation/dsp_fir_decimator_float_complex.h"
#include "dsp/dsp_buffers.h"
#include "dsp/fft/fft.h"
#include "dsp/fft/fft_types.h"
#include "dsp/fir_filter.h"
#include "dsp/dsp_common.h"
#include "dsp/modulation/dsp_demodulate.h"
#include "main_board.h"
#include "radio.h"
#include "status.h"
#include "hw/stm32f4xx/timers.h"
#include "config.h"
#include "FIFO.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "types.h"
#include "ui/view.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include "dsp/decimation/dsp_decimators.h"
#include <cstddef>
#include <memory>
#include <sys/_stdint.h>

#include "printf.h"
#include "utils.hpp"

/* Should be defined in the HW abstraction layer */
extern TIM_HandleTypeDef TASKS_TIMER_HANDLE;

void ReceiveTaskBase::work() {
    if (status.status == DSP_STATUS_RUNNING) {

        char *in_start;
        char *in_p;
        char *out_p;

        uint32_t free = output_stream.free(&out_p);
        uint32_t av = input_stream.available(&in_p);
        uint32_t output_samples = 0;
        float32_t *tmp_out = out_f32_p;

        if (av >= DSP_FIFO_BLOCK_BYTES && free >= (DSP_FIFO_BLOCK_BYTES / status.decimation_factor)) {

            this->status.processed_blocks++;
            av = DSP_FIFO_BLOCK_BYTES;

            in_start = in_p;

            // Process blocks (DSP_BLOCK items each)
            while (av >= bytes_per_batch) {

                uint16_t block_size_in = samples_per_batch;
                uint16_t block_size_out = samples_per_batch / status.decimation_factor;

                dsp::s16_to_f32((const adc_type *)in_p, bi2_p, block_size_in << 1);

                dsp::unzip_f32((const float32_t *)bi2_p, bi1_p, bq1_p, block_size_in);

                for (int i = 0; i < n_decimators; i++) {

                    if (i < n_decimators - 1) {
                        // Half-band decimators
                        decimators[i].decimate(bi1_p, bq1_p, bi2_p, bq2_p, block_size_in);
                        block_size_in /= decimators[i].get_factor();
                    } else {
                        // Output decimator. This is the final nawrrowband decimator
                        signal_decimator->decimate(bi1_p, bq1_p, tmp_out, tmp_out + samples_per_batch, block_size_in);
                    }

                    SWAP_PTR(bi1_p, bi2_p);
                    SWAP_PTR(bq1_p, bq2_p);
                }

                tmp_out += block_size_out;
                output_samples += block_size_out;

                if (output_samples == samples_per_batch) {

                    tmp_out = out_f32_p;
                    dsp::zip_f32(tmp_out, tmp_out + samples_per_batch, (float32_t *)bi2_p, samples_per_batch);

#if !DSP_FS4_SHIFT
                    // DC block
                    buffer_t<float32_t> bb = {(float32_t *)bi2_p, (size_t)block_size_out << 1};
                    dc_block_i.filter(bb, 2, 0);
                    dc_block_q.filter(bb, 2, 1);
#endif

                    // Wrap the destination buffer
                    buffer_t<complex_t_f32> buff_out = {(complex_t_f32 *)bi2_p, (size_t)samples_per_batch};
                    buffer_t<float32_t> buff_out_f32 = {(float32_t *)bi1_p, (size_t)samples_per_batch << 1, status.sample_rate};

                    demodulator->work(buff_out, (float32_t *)buff_out_f32.p);

                    process_audio(buff_out_f32);

                    dsp::f32_to_s16((const float32_t *)bi1_p, (adc_type *)out_p, samples_per_batch << 1);

                    out_p += bytes_per_batch;
                    output_samples = 0;
                }

                in_p += bytes_per_batch;
                av -= bytes_per_batch;
            }

            uint32_t processed = DSP_FIFO_BLOCK_BYTES - av;

            input_stream.consume(processed, &in_start);
            output_stream.feed(processed / status.decimation_factor);

        } else {
            if (free < (DSP_FIFO_BLOCK_BYTES / status.decimation_factor)) {
                status.fifo_overruns++;
            }
            if (av < DSP_FIFO_BLOCK_BYTES) {
                status.fifo_underruns++;
            }
        }
    }
}

bool ReceiveTaskBase::init_decimators(MODULATION_MODE mod) {
    // First staes are half-band filters (https://en.wikipedia.org/wiki/Half-band_filter)
    uint8_t factor;
    uint8_t dec = status.decimation_factor;

    int32_t next_stage_bandwidth;
    uint32_t stage_fs = config.fft.sample_rate;
    n_decimators = 0;

    bool ret;
    while (dec > 1) {

        if (n_decimators == max_decimators - 1 || dec == 2) { // || (stage_fs / factor) > (status.bandwidth / 2)) {

            // Final narrowband signal decimator
            factor = dec;
            next_stage_bandwidth = status.bandwidth;

            switch (mod) {
                case SSB_USB:
                case SSB_LSB:

                    signal_decimator = std::make_unique<DspFIRDecimatorFloatComplex<FIR_DECIMATOR_SIGNAL_TAPS>>();
                    ret = signal_decimator->config(stage_fs, next_stage_bandwidth, factor);
                    break;
                case CW:
                    signal_decimator = std::make_unique<DspFIRDecimatorFloatComplex<FIR_DECIMATOR_SIGNAL_TAPS>>();
                    // TODO: Select pitch (offset center freq)
                    ret = signal_decimator->config(stage_fs, next_stage_bandwidth, factor, max2(CW_PITCH_HZ - (next_stage_bandwidth / 2), 0));
                    break;
                default:
                    signal_decimator = std::make_unique<DspFIRDecimatorFloat<FIR_DECIMATOR_SIGNAL_TAPS, complex_t_f32>>();
                    ret = signal_decimator->config(stage_fs, next_stage_bandwidth / 2, factor);
                    break;
            }

        } else {
            factor = dec > 16 ? 8 : (dec > 8 ? 4 : 2);
            next_stage_bandwidth = (stage_fs / (factor * 2));
            ret = decimators[n_decimators].config(stage_fs, next_stage_bandwidth, factor);
        }

        if (!ret) {
            return false;
        }

        dec /= factor;
        n_decimators++;
        stage_fs = stage_fs / factor;
    }

    dec = dec / factor;
    return true;
}

std::unique_ptr<dsp::demodulator> ReceiveTaskBase::get_modulator() {

    std::unique_ptr<dsp::demodulator> demod;
    switch (get_modulation_mode()) {
        case AM:
            return std::make_unique<dsp::am_demodulator>();
        case CW:
        case SSB_LSB:
        case SSB_USB:
            return std::make_unique<dsp::ssb_demodulator>();
        case FM:
            demod = std::make_unique<dsp::fm_demodulator>();
            ((dsp::fm_demodulator *)demod.get())->configure(status.sample_rate, 2500);
            return demod;
        case WFM:
            demod = std::make_unique<dsp::fm_demodulator>();
            ((dsp::fm_demodulator *)demod.get())->configure(status.sample_rate, 75000);
            return demod;
        default:
            return std::make_unique<dsp::ssb_demodulator>();
    }
}

bool ReceiveTaskBase::start() {

    main_board::setMute(GPIO_PIN_SET);

    // Stop task processing timer (in case this is a restart)
    HAL_TIM_Base_Stop_IT(&TASKS_TIMER_HANDLE);

    dsp_set_real_time(true);

    int dec_factor = 1;
    status.sample_rate = config.fft.sample_rate;
    // calculate decimation ratio to get to audio bandwidth
    while (status.sample_rate > get_audio_bw_hz() * 2 && dec_factor < 32) {
        dec_factor <<= 1;
        status.sample_rate /= 2;
    }

    status.direction = DSP_DIRECTION_INOUT;

    MODULATION_MODE mod = get_modulation_mode();

    status.bandwidth = radio::get_bandwidth_hz();

    status.decimation_factor = dec_factor;
    status.bits_per_sample = sizeof(adc_type) * 8;
    status.n_channels = 2;
    status.block_size_bytes = DSP_BLOCK * sizeof(complex_t);
    status.decimated_block_size = DSP_BLOCK / dec_factor;
    status.decimated_block_size_bytes = status.decimated_block_size * sizeof(complex_t);

    bool ret = init_decimators(mod);

    if (!ret) {
        halt(DSP_ERR);
        return false;
    }

    demodulator = get_modulator();

    ret = init();

    ret = radio_config({.direction = RF_DIRECTION_RX,
                        .sample_freq = status.sample_rate,
                        .freq = 0,
                        .mode = DSP}); // Radio mode is DSP so the signal is routed to the audio amp

    if (!ret) {
        halt(DSP_ERR);
        return false;
    }

    main_board::setMute(GPIO_PIN_RESET);

    // Start task processing timer
    // TODO: This should be done by the caller of this method and be generic for all tasks
    HAL_TIM_Base_Start_IT(&TASKS_TIMER_HANDLE);

    // Se the fifo processing frequency
    update_timer(TASKS_TIMER_TYPEDEF, 80, TASKS_TIMER_TYPEDEF_CLOCK_HZ / 100000);
    status.status = DSP_STATUS_RUNNING;
    return true;
}

void ReceiveTaskBase::stop() {

    if (status.status != DSP_STATUS_STOPPED) {

        status.status = DSP_STATUS_STOPPED;

        dsp_set_real_time(false);

        // Stop task processing timer
        HAL_TIM_Base_Stop_IT(&TASKS_TIMER_HANDLE);

        Task::stop(); // Let the base class finish

        radio_config({RF_DIRECTION_RX, 0});
    }
}
