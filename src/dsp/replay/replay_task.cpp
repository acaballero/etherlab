//
// Created by Angel Dust on 16/04/2021.
//

#include "replay_task.h"
#include "arm_math.h"
#include "dsp/blocks/signal_generator.h"
#include "dsp/buffer.hpp"
#include "dsp/dsp_buffers.h"
#include "dsp/fir_filter.h"
#include "dsp/dsp_common.h"
#include "dsp/interpolation/dsp_fir_interpolator_q15.h"
#include "main_board.h"
#include "status.h"
#include "hw/stm32f4xx/timers.h"
#include "config.h"
#include "FIFO.h"
#include "types.h"
#include "ui/view.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include <cstring>
#include <memory>

/* Should be defined in the HW abstraction layer */
extern TIM_HandleTypeDef TASKS_TIMER_HANDLE;

void ReplayTask::fetch() {

    bool eof = false;
    char *in_p;
    int32_t input_free = input_stream.free(&in_p);
    int32_t in_bytes = DSP_FIFO_BLOCK_BYTES;

    if (input_free >= in_bytes) {

        if (FatFSFileHandle.fptr < FatFSFileHandle.fsize - in_bytes) {

            FRESULT fres = m_file->read(in_p, in_bytes);

            if (fres == FR_OK) {

                input_stream.feed(in_bytes);

            } else if (fres != FR_DISK_ERR || this->info.status == DSP_STATUS_RUNNING) {

                // We check again for the status because the ADC interrupt could've stopped the capture before
                // TODO: do better error handling
                if (FatFSFileHandle.fptr < FatFSFileHandle.fsize - in_bytes) {
                    this->abort(DSP_ERR_FILEREAD);

                } else {
                    eof = true;
                }
            }
        } else {
            eof = true;
        }

        if (eof) {

            if (!this->loop) {
                this->stop();
            } else {
#if LCD_DISABLE_ON_DSP
                // Show at least the result of each replay
                lcd.setEnabled(true);
                view_manager::mainView.paint();
                lcd.setEnabled(false);
#endif
                f_rewind(&FatFSFileHandle);
                this->reset();
            }
        }
    }
}

dsp::SignalGenerator sig_gen(10000, 165000, dsp::SIGNAL_SHAPE_SIN);

void ReplayTask::produce() {

    char *in_p;
    char *out_p;

    int32_t input_av = input_stream.available(&in_p);
    input_av -= consumed_in_bytes;

    uint32_t output_free = output_stream.free(&out_p);
    uint32_t in_samples = samples_per_batch / interpolator->get_factor();

    uint32_t out_bytes = DSP_FIFO_BLOCK_BYTES;
    int32_t in_bytes = out_bytes / interpolator->get_factor();

    if (output_free >= out_bytes && input_av >= in_bytes) {

        in_p += consumed_in_bytes;

        auto bytes_left = in_bytes;

        // Process blocks (DSP_BLOCK items each)
        while (bytes_left > 0) {
            GPIOD->BSRR = GPIO_PIN_9;

            ////// DEBUG /////////
            // complex_t sample;

            // sig_gen.set_gain_db(-20);
            // for (size_t i = 0; i < in_samples * 2; i += 2) {
            //     sig_gen.get_complex_sample(sample);
            //     ((adc_type *)in_p)[i] = sample.i;
            //     ((adc_type *)in_p)[i + 1] = sample.r;
            // }
            //// DEBUG ////

            dsp::s16_to_f32((const adc_type *)in_p, f32_in, in_samples << 1);

            buffer_t<float32_t> src = {f32_in, in_samples, 0, COMPLEX_INTERLEAVED};
            buffer_t<float32_t> dst = {f32_out, samples_per_batch, 0, COMPLEX_INTERLEAVED};

            if (interpolator->get_factor() == 1) {
                memcpy(out_p, in_p, bytes_per_batch);
            } else {
                interpolator->interpolate(src, dst);
            }

            dsp::f32_to_s16(f32_out, (adc_type *)out_p, samples_per_batch << 1);
            GPIOD->BSRR = GPIO_PIN_9 << 16;

            output_stream.feed(bytes_per_batch);

            bytes_left -= bytes_per_batch / interpolator->get_factor();
            in_p += bytes_per_batch / interpolator->get_factor();

            out_p += bytes_per_batch;
        }

        if (++info.processed_blocks == 1 && on_first_block) {
            on_first_block();
        }

        consumed_in_bytes += in_bytes;

        if (consumed_in_bytes == DSP_FIFO_BLOCK_BYTES) {
            // The FIFOs need to be consumed and feed in blocks of the same size
            input_stream.consume(consumed_in_bytes, &in_p);
            consumed_in_bytes = 0;
        }
    }
}

void ReplayTask::work() {

    if (this->info.status != DSP_STATUS_RUNNING) {
        return;
    }

    produce();
    fetch();
}

bool ReplayTask::start_impl() {

#if LCD_DISABLE_ON_DSP
    lcd.setEnabled(false);
#endif

#if DSP_USE_FIR_FILTER
    // Clear the state of the decimate instance
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(dsp_firStateBuffer));
#else
#endif

    // Result after operations
    FRESULT fres;

    // Read the header info block
    WaveInfo wi;

    info.reset();

    fres = m_file->open(wi);

    main_board::set_mode(DIGITAL_TX);

    dsp_set_real_time(true);

    if (fres != FR_OK) {

        this->abort(DSP_ERR_FILEREAD);
        return false;

    } else {

        // LOG(
        //         "Info header:\nChannels:%d\nBits per sample :%u\nByte Rate:%lu\nCarrier:%llu\nFormat:%u\nSample rate:%lu\n",
        //         wi.n_channels, wi.bits_sample, wi.byte_rate, wi.carrier_freq, wi.format, wi.sample_rate)

        uint8_t decimation_factor = 1;

        if (wi.sample_rate) { // The stored file has sample rate information

            // Update FFT  sample rate parameters to force match the recorded signal

            dsp::enable_freq_mult(false);
            uint8_t max_decimation_factor = 2;
            dsp::set_max_decimation(max_decimation_factor);
            fft::set_max_decimation(max_decimation_factor);

            fft_config(wi.sample_rate * USABLE_BW_FACTOR);

            uint32_t sf = fft::fft_params.sample_freq;

            // Note we don't consider stored signal sample rates higher than the FFT sample frequency

            while (decimation_factor < max_decimation_factor && sf >= wi.sample_rate * 2) {
                decimation_factor <<= 1;
                sf >>= 1;
            }

            // TODO: Since we can't currently have a decimation factor other than a power of two, this calculation
            // can result in a frequency shift with respect to the one the signal was stored. Find a way to match it in every case
            if (sf != wi.sample_rate) {
                status::pop_alert(status::WARN, "Unable to match wave sample rate");
            }

        } else {

            // Since we don't have the sample rate of the stored waveform, we'll set it to the effective sample rate of the fft processing
            wi.sample_rate = fft::fft_params.sample_freq / fft::fft_params.decimation_factor;
            wi.carrier_freq = radio::get_frequency();
            decimation_factor = fft::fft_params.decimation_factor;
        }

        this->info.direction = DSP_DIRECTION_OUT;
        this->info.bandwidth = fft::fft_params.span;
        this->info.sample_rate = fft::fft_params.sample_freq;
        this->info.decimation_factor = fft::fft_params.decimation_factor;
        this->info.bits_per_sample = wi.bits_sample;
        this->info.n_channels = wi.n_channels; // I/Q
        this->info.block_size_bytes = DSP_BLOCK * 2 * 2;
        this->info.decimated_block_size = DSP_BLOCK / decimation_factor / (this->info.n_channels == 1 ? 2 : 1);
        this->info.decimated_block_size_bytes = this->info.block_size_bytes / decimation_factor / (this->info.n_channels == 1 ? 2 : 1);

        interpolator = std::make_unique<DspFIRInterpolatorFloat<FIR_INTERPOLATOR_BASEBAND_TAPS>>();
        bool ret = interpolator->config(wi.sample_rate, info.sample_rate / 4, decimation_factor);

        if (!ret) {
            this->abort(DSP_ERR);
            return false;
        }

        // Set this task frequency

        update_timer(TASKS_TIMER_TYPEDEF, 2, TASKS_TIMER_TYPEDEF_CLOCK_HZ / 10000);

#ifdef __STM32F3xx_HAL_H
        // In STM32F3xx ,only as a POC, we will use only one DAC, so are limited to play only one channel
        this->status.n_channels = 1;
#endif

        // The signal samples will be interpolated by the current decimation factor to adapt the rate
        // to that of the FFT processing chain (which will in turn downsample them by the same factor)
        // So we write samples to the DAC at a rate equal to the desired sample rate multiplied
        // by the interpolation (->DAC) or decimation (ADC->) factor
        ret = radio_config({.direction = RF_DIRECTION_TX, .sample_freq = info.sample_rate, .freq = 0, .mode = DSP});

        // TODO: Manage gain globally. Not that easy considering in receive we'd need to normalize it and that's not easy for all modulations

        this->info.status = DSP_STATUS_RUNNING;
        if (!ret) {
            this->abort(DSP_ERR);
            return false;
        }

        // Capture/Replay won't apply frequency shifts (for DC issues mitigation) because:
        // - The ReplayProcessor is also used in other tasks (e.g. APRS trasnsmit) that do not accout for the shift
        // - Honestly I haven't take the time to think about why it would benefit from the shifting, but it is very likely
        //   required since the DC blocker is surely killing whatever is at DC (the carrier itself if any)
        // dsp::enable_frequency_shift(false);
    }

    return true;
}

void ReplayTask::stop() {

    if (this->info.status != DSP_STATUS_STOPPED) {

        output_stream.close();

        this->info.status = DSP_STATUS_STOPPED;

        FRESULT fres; // Result after operations

        fres = m_file->close();

        if (fres != FR_OK) {
            if (this->info.error != DSP_ERR_NONE) {
                info.error = DSP_ERR_FILECLOSE;
            }
        }

        // Stop task trigger timer
        HAL_TIM_Base_Stop_IT(&TASKS_TIMER_HANDLE);

        dsp::set_max_decimation(MAX_DSP_DECIMATION_FACTOR);
        dsp::enable_freq_mult(true);
        dsp_set_real_time(false);
        // dsp::enable_frequency_shift(true);

        radio_config({.direction = RF_DIRECTION_RX, .sample_freq = 0, .freq = 0, .mode = DSP});

        Task::stop(); // Let the base class finish

#if LCD_DISABLE_ON_DSP
        lcd.setEnabled(true);
#endif
    }
}

void ReplayTask::setFile(File *file) {
    m_file = file;
    m_file->close();
}

bool ReplayTask::getLoop() const {
    return loop;
}

void ReplayTask::setLoop(bool b) {
    ReplayTask::loop = b;
}
