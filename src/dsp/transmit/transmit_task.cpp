//
// Created by Angel Dust on 21/01/2026.
//

#include "transmit_task.h"
#include "arm_math.h"
#include "dsp/blocks/dc_block.h"
#include "dsp/buffer.hpp"
#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "dsp/decimation/dsp_fir_decimator_float_complex.h"
#include "dsp/dsp_buffers.h"
#include "dsp/fft/fft.h"
#include "dsp/fft/fft_params.h"
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
#include "tinyusb/tusb_config.h"
#include "tinyusb/usb_audio_dsp_bridge.h"
#include "types.h"
#include "ui/view.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include <cstddef>
#include <cstring>
#include <memory>
#include <sys/_stdint.h>

#include "printf.h"
#include "utils.hpp"

/* Should be defined in the HW abstraction layer */
extern TIM_HandleTypeDef TASKS_TIMER_HANDLE;

HOT_FUNCTION
void TransmitTask::work() {
    if (status.status == DSP_STATUS_RUNNING) {

        char *in_start;
        char *in_p;
        char *out_p;

        uint32_t free = output_stream.free(&out_p);
        uint32_t av = input_stream.available(&in_p);
        uint32_t output_samples = 0;
        uint32_t required_av;
        uint32_t required_free;
        uint32_t n_in;
        uint32_t n_in_bytes;
        uint32_t n_out;

        // This consumes real-values and produces complex samples, so twice the required available size

        if (interpolator) {
            required_av = DSP_FIFO_BLOCK_BYTES / 2 / status.decimation_factor;
            required_free = required_av * 2;
            n_in = status.decimated_block_size;
            n_in_bytes = status.decimated_block_size_bytes / 2;
            n_out = samples_per_batch;

        } else {
            required_av = DSP_FIFO_BLOCK_BYTES / 2;
            required_free = required_av * 2 / status.decimation_factor;
            n_in = samples_per_batch;
            n_in_bytes = bytes_per_batch_real;
            n_out = status.decimated_block_size;
        }

        if (av >= required_av && free >= required_free) {

            status.processed_blocks++;
            av = required_av;
            in_start = in_p;

            // Process blocks (DSP_BLOCK items each)
            while (av >= n_in_bytes) {

                dsp::s16_to_f32((const adc_type *)in_p, bi1_p, n_in);

                buffer_t<float32_t> src = {bi1_p, n_in, REAL};
                buffer_t<float32_t> dst = {out_accum_p, n_out, REAL};

                if (interpolator) {
                    interpolator->interpolate(src, dst);
                } else if (decimator) {
                    decimator->decimate(src, dst);
                }

                //       demodulator->work_real(half_accum_buff_f32_p, half_accum_buff_f32_p + samples_per_batch, bi1_p, samples_per_batch);

                output_samples += n_out;
                out_accum_p += n_out;

                if (output_samples == samples_per_batch) {

                    out_accum_p = bq1_p;

                    if (!baseband_echo) {
                        buffer_t<float32_t> buff_out_f32 = {out_accum_p, (size_t)samples_per_batch, status.sample_rate, REAL};
                        process_audio(buff_out_f32);
                    }

                    dsp::f32_to_s16((const float32_t *)out_accum_p, (adc_type *)out_p, samples_per_batch);

                    out_p += bytes_per_batch_real;
                    output_samples = 0;
                    x output_stream.feed(bytes_per_batch_real);
                }

                in_p += n_in_bytes;
                av -= n_in_bytes;
            }

            uint32_t processed = required_av - av;
            input_stream.consume(processed, &in_start);

            if (status.processed_blocks == 1 && on_first_block) {
                on_first_block();
            }

        } else {
            if (free < required_free) {
                status.fifo_overruns++;
            }
            if (av < required_av) {
                status.fifo_underruns++;
            }
        }
    }
}

bool TransmitTask::init_resampler(MODULATION_MODE mod) {

    bool ret;

    decimator.reset();
    interpolator.reset();

    if (status.sample_rate <= DSP_AUDIO_SAMPLE_RATE) { // Even it no data conversion is required, the decimator serves as filter
        decimator = std::make_unique<DspFIRDecimatorFloat<FIR_DECIMATOR_SIGNAL_TAPS>>();
        ret = decimator->config(status.sample_rate * status.decimation_factor, status.bandwidth,
                                status.decimation_factor); // Here the bandwidth is halved for double sideband modulations

    } else {
        interpolator = std::make_unique<DspFIRInterpolatorFloat<FIR_DECIMATOR_SIGNAL_TAPS>>();
        ret = interpolator->config(status.sample_rate / status.decimation_factor, status.bandwidth,
                                   status.decimation_factor); // Here the bandwidth is halved for double sideband modulations
    }

    if (!ret) {
        LOG("Error configuring resampler for mod:%d, fs:%d, bw:%d, factor:%d\n", mod, status.sample_rate, status.bandwidth, status.decimation_factor);
        return false;
    }

    LOG("Rate %d -> %d (filter: %d)\n",
        status.sample_rate <= DSP_AUDIO_SAMPLE_RATE ? status.sample_rate * status.decimation_factor : status.sample_rate / status.decimation_factor,
        status.sample_rate, status.bandwidth);

    return true;
}

// std::unique_ptr<dsp::demodulator> TransmitTask::get_modulator() {

//     std::unique_ptr<dsp::demodulator> demod;

//     auto mode = get_modulation_mode();

//     if (get_baseband_echo() || mode == NONE) {
//         return std::make_unique<dsp::ssb_demodulator>();
//     } else {
//         switch (mode) {
//             case AM:
//                 return std::make_unique<dsp::am_demodulator>();
//             case CW:
//             case SSB_LSB:
//             case SSB_USB:
//                 return std::make_unique<dsp::ssb_demodulator>();
//             case FM:
//                 demod = std::make_unique<dsp::fm_demodulator>();
//                 ((dsp::fm_demodulator *)demod.get())->configure(demodulation_sample_rate, config.dsp.fm_max_deviation);
//                 return demod;
//             case WFM:
//                 demod = std::make_unique<dsp::fm_demodulator>();
//                 ((dsp::fm_demodulator *)demod.get())->configure(demodulation_sample_rate, config.dsp.wideband_fm_max_deviation);
//                 return demod;
//             default:
//                 return std::make_unique<dsp::ssb_demodulator>();
//         }
//     }
// }

bool TransmitTask::start() {

    LOG("___ [START] Transmit task ___\n");

    set_baseband_echo(dsp::dsp_config.baseband_echo);

    // Stop task processing timer (in case this is a restart)
    HAL_TIM_Base_Stop_IT(&TASKS_TIMER_HANDLE);

    dsp_set_real_time(true);

    status.sample_rate = DSP_AUDIO_SAMPLE_RATE;

    uint32_t dac_sample_rate = fft::fft_params.sample_freq;

    modulation_bandwidth_hz = get_modulation_bw_hz();

    MODULATION_MODE mod = get_modulation_mode();

    main_board::set_modulation_mode(mod, false);

    if (mod != SSB_USB && mod != SSB_LSB && mod != CW) {
        modulation_bandwidth_hz /= 2; // Halved for double sideband modulations since modulation bandwidth represents the double sideband bandwidth
    }

    // Calculate decimation ratio to get as closest as possible to our target audio bandwidth
    // (while using decimation factors of 2^n)
    int dec_factor = 1;
    while (status.sample_rate > dac_sample_rate && dec_factor < MAX_DSP_DECIMATION_FACTOR) {
        dec_factor <<= 1;
        status.sample_rate /= 2;
    }

    // If target sample rate is higher: interpolate
    while (status.sample_rate < dac_sample_rate && dec_factor < MAX_DSP_DECIMATION_FACTOR) {
        dec_factor <<= 1;
        status.sample_rate *= 2;
    }

    status.bandwidth = modulation_bandwidth_hz;

    status.direction = DSP_DIRECTION_OUT;

    status.decimation_factor = dec_factor;
    status.bits_per_sample = sizeof(adc_type) * 8;
    status.n_channels = 2;
    status.block_size_bytes = DSP_BLOCK * sizeof(complex_t);
    status.decimated_block_size = DSP_BLOCK / dec_factor;
    status.decimated_block_size_bytes = status.decimated_block_size * sizeof(complex_t);

    LOG("Bandwidth: %d | Sample rate: %d | Modulation bandwidth: %d\n", status.bandwidth, status.sample_rate, modulation_bandwidth_hz);

    bool ret = init_resampler(mod);

    if (!ret) {

        halt(DSP_ERR);
        return false;
    }

    // demodulator = get_modulator();

    out_accum_p = bq1_p;

    ret = init();

    ret = ret && radio_config({.direction = RF_DIRECTION_TX,
                               .sample_freq = status.sample_rate,
                               .freq = 0,
                               .mode = DSP}); // Radio mode is DSP so the signal is routed to the audio amp

    if (!ret) {

        halt(DSP_ERR);
        return false;
    }

    // Start task processing timer
    // TODO: This should be done by the caller of this method and be generic for all tasks
    HAL_TIM_Base_Start_IT(&TASKS_TIMER_HANDLE);

    // Se the fifo processing frequency
    update_timer(TASKS_TIMER_TYPEDEF, 40, TASKS_TIMER_TYPEDEF_CLOCK_HZ / 100000);
    status.status = DSP_STATUS_RUNNING;
    return true;
}

void TransmitTask::stop() {

    if (status.status != DSP_STATUS_STOPPED) {
        // Stop task processing timer
        HAL_TIM_Base_Stop_IT(&TASKS_TIMER_HANDLE);

        // Free decimators memory (wish this wouldn't be necessary but there must be room for other allocations while stopped)

        // decimator.reset();

        status.status = DSP_STATUS_STOPPED;

        dsp_set_real_time(false);

        Task::stop(); // Let the base class finish

        radio_config({RF_DIRECTION_RX, 0});
    }
}
