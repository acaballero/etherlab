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
#include "dsp/modulation/dsp_modulate.h"
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
    if (info.status == DSP_STATUS_RUNNING) {

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
        float32_t s16_scale = 1 / 2048.0f; // 12 bits precission for the DAC
        float32_t s16_scale_inv = 2048.0f;

        MODULATION_MODE mode = get_modulation_mode();

        // This consumes real-values and produces complex samples, so twice the required available size

        if (interpolator) {
            required_av = DSP_FIFO_BLOCK_BYTES / 2 / info.decimation_factor;
            required_free = required_av * 2;
            n_in = info.decimated_block_size;
            n_in_bytes = info.decimated_block_size_bytes / 2;
            n_out = samples_per_batch;

        } else {
            required_av = DSP_FIFO_BLOCK_BYTES / 2;
            required_free = required_av * 2 / info.decimation_factor;
            n_in = samples_per_batch;
            n_in_bytes = bytes_per_batch_real;
            n_out = info.decimated_block_size;
        }
        GPIOD->BSRR |= GPIO_PIN_9;
        if (av >= required_av && free >= required_free) {

            info.processed_blocks++;
            av = required_av;
            in_start = in_p;

            // Process blocks (DSP_BLOCK items each)
            while (av >= n_in_bytes) {

                dsp::s16_to_f32_norm((const adc_type *)in_p, bi1_p, n_in, s16_scale);

                buffer_t<float32_t> src = {bi1_p, n_in, 0, REAL};
                buffer_t<float32_t> dst = {out_accum_p, n_out, 0, REAL};

                if (interpolator) {
                    interpolator->interpolate(src, dst);
                } else if (decimator) {
                    decimator->decimate(src, dst);
                }

                output_samples += n_out;
                out_accum_p += n_out;

                if (output_samples == samples_per_batch) {

                    out_accum_p = bq1_p;

                    buffer_t<complex_t_f32> out_buffer = {(complex_t_f32 *)bi1_p, samples_per_batch, COMPLEX_INTERLEAVED};

                    buffer_t<float32_t> src = {out_accum_p, samples_per_batch, REAL};

                    high_pass_filter.decimate(src, src, 0, 1, 1);

                    if (get_baseband_echo() || mode == NONE) {
                        // Just copy the unmodulated buffer
                        dsp::f32_to_s16_norm((const float32_t *)out_accum_p, (adc_type *)out_p, samples_per_batch, s16_scale_inv);
                        // Convert to interleaved format
                        dsp::zip_c16((const adc_type *)out_p, (adc_type *)out_p + samples_per_batch, (adc_type *)out_p, samples_per_batch << 1);

                    } else {
                        modulator->work(out_accum_p, out_buffer);
                        dsp::f32_to_s16_norm((const float32_t *)bi1_p, (adc_type *)out_p, samples_per_batch << 1, s16_scale_inv);
                    }

                    out_p += bytes_per_batch;
                    output_samples = 0;
                    output_stream.feed(bytes_per_batch);
                }

                in_p += n_in_bytes;
                av -= n_in_bytes;
            }

            uint32_t processed = required_av - av;
            input_stream.consume(processed, &in_start);

            if (info.processed_blocks == 1 && on_first_block) {
                on_first_block();
            }

        } else {
            if (free < required_free) {
                info.fifo_overruns++;
            }
            if (av < required_av) {
                info.fifo_underruns++;
            }
        }
        GPIOD->BSRR |= GPIO_PIN_9 << 16;
    }
}

bool TransmitTask::init_resampler(MODULATION_MODE mod) {

    bool ret;

    decimator.reset();
    interpolator.reset();

    if (info.sample_rate <= USB_AUDIO_SAMPLE_RATE) { // Even it no data conversion is required, the decimator serves as filter
        decimator = std::make_unique<DspFIRDecimatorFloat<FIR_DECIMATOR_SIGNAL_TAPS>>();
        ret = decimator->config(info.sample_rate * info.decimation_factor, info.bandwidth,
                                info.decimation_factor); // Here the bandwidth is halved for double sideband modulations

    } else {
        interpolator = std::make_unique<DspFIRInterpolatorFloat<FIR_INTERPOLATOR_BASEBAND_TAPS>>();
        ret = interpolator->config(info.sample_rate / info.decimation_factor, info.bandwidth,
                                   info.decimation_factor); // Here the bandwidth is halved for double sideband modulations
    }

    if (!ret) {
        LOG("Error configuring resampler for mod:%d, fs:%d, bw:%d, factor:%d\n", mod, info.sample_rate, info.bandwidth, info.decimation_factor);
        return false;
    }

    LOG("Rate %d -> %d (filter: %d)\n",
        info.sample_rate <= USB_AUDIO_SAMPLE_RATE ? info.sample_rate * info.decimation_factor : info.sample_rate / info.decimation_factor, info.sample_rate,
        info.bandwidth);

    return true;
}

std::unique_ptr<dsp::modulator> TransmitTask::get_modulator() {

    std::unique_ptr<dsp::modulator> mod;
    bool ret = false;
    auto mode = get_modulation_mode();

    if (get_baseband_echo() || mode == NONE) {
        return nullptr;
    } else {
        switch (mode) {
            case AM:

                mod = std::make_unique<dsp::am_modulator>();
                ret = true;
                break;
            case CW:
                mod = std::make_unique<dsp::ssb_modulator>(dsp::ssb_modulator::USB);
                ret = ((dsp::ssb_modulator *)mod.get())->configure(info.sample_rate, info.bandwidth);
                break;
            case SSB_LSB:
                mod = std::make_unique<dsp::ssb_modulator>(dsp::ssb_modulator::LSB);
                ret = ((dsp::ssb_modulator *)mod.get())->configure(info.sample_rate, info.bandwidth);
                break;
            case SSB_USB:
                mod = std::make_unique<dsp::ssb_modulator>(dsp::ssb_modulator::USB);
                ret = ((dsp::ssb_modulator *)mod.get())->configure(info.sample_rate, info.bandwidth);
                break;
            case FM:
                mod = std::make_unique<dsp::fm_modulator>();
                ret = true;
                ((dsp::fm_modulator *)mod.get())->configure(info.sample_rate, config.dsp.fm_max_deviation);
                break;

            default:
                status::pop_alert(status::ERROR, "Transmit task: Unsupported DSP modulation mode configuring modulator");
                return nullptr;
        }

        if (!ret) {
            status::pop_alert(status::ERROR, "Transmit task: Error configuring DSP modulator");
        }

        return mod;
    }
}

bool TransmitTask::start_impl() {

    LOG("___ [START] Transmit task ___\n");

    set_baseband_echo(dsp::dsp_config.baseband_echo);

    // Stop task processing timer (in case this is a restart)
    HAL_TIM_Base_Stop_IT(&TASKS_TIMER_HANDLE);

    dsp_set_real_time(true);

    info.sample_rate = USB_AUDIO_SAMPLE_RATE; // Start at the USB audio rate. Will bring it down/up to the FFT sample rate (determined in main_board.cpp and
                                              // depending on the mode being TX or RX)

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
    while (info.sample_rate > dac_sample_rate && dec_factor < dsp::get_max_decimation()) {
        dec_factor <<= 1;
        info.sample_rate /= 2;
    }

    // If target sample rate is higher: interpolate
    while (info.sample_rate < dac_sample_rate && dec_factor < dsp::get_max_decimation()) {
        dec_factor <<= 1;
        info.sample_rate *= 2;
    }

    high_pass_filter.config(info.sample_rate, 300, 1, HPF);

    info.bandwidth = modulation_bandwidth_hz;

    info.direction = DSP_DIRECTION_OUT;

    info.decimation_factor = dec_factor;
    info.bits_per_sample = sizeof(adc_type) * 8;
    info.n_channels = 2;
    info.block_size_bytes = DSP_BLOCK * sizeof(complex_t);
    info.decimated_block_size = DSP_BLOCK / dec_factor;
    info.decimated_block_size_bytes = info.decimated_block_size * sizeof(complex_t);

    LOG("Bandwidth: %d | Sample rate: %d\n", info.bandwidth, info.sample_rate);

    bool ret = init_resampler(mod);

    if (!ret) {

        abort(DSP_ERR);
        return false;
    }

    modulator = get_modulator();

    out_accum_p = bq1_p;

    ret = init();

    ret = ret && radio_config({.direction = RF_DIRECTION_TX,
                               .sample_freq = info.sample_rate,
                               .freq = 0,
                               .mode = DSP}); // Radio mode is DSP so the signal is routed to the audio amp

    if (!ret) {

        abort(DSP_ERR);
        return false;
    }

    // Set the fifo processing frequency
    update_timer(TASKS_TIMER_TYPEDEF, 40, TASKS_TIMER_TYPEDEF_CLOCK_HZ / 100000);
    info.status = DSP_STATUS_RUNNING;
    return true;
}

void TransmitTask::stop() {

    if (info.status != DSP_STATUS_STOPPED) {
        // Stop task processing timer
        HAL_TIM_Base_Stop_IT(&TASKS_TIMER_HANDLE);

        // Free decimators memory (wish this wouldn't be necessary but there must be room for other allocations while stopped)

        decimator.reset();
        interpolator.reset();

        info.status = DSP_STATUS_STOPPED;

        dsp_set_real_time(false);

        Task::stop(); // Let the base class finish

        radio_config({RF_DIRECTION_RX, 0});
    }
}
