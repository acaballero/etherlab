//
// Created by Angel Dust on 16/04/2021.
//

#include "replay_task.h"
#include "dsp/dsp_buffers.h"
#include "dsp/fir_filter.h"
#include "dsp/dsp_common.h"
#include "status.h"
#include "hw/stm32f4xx/timers.h"
#include "config.h"
#include "FIFO.h"
#include "ui/view.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"

/* Should be defined in the HW abstraction layer */
extern TIM_HandleTypeDef TASKS_TIMER_HANDLE;

void ReplayTask::work() {

    if (this->info.status == DSP_STATUS_RUNNING) {

        // UINT bytesRead;
        char *p;
        bool eof = false;

        uint32_t free = output_stream.free(&p);

        if (free >= DSP_FIFO_BLOCK_BYTES) {

            // GPIOA->BSRR = GPIO_PIN_12;

            if (FatFSFileHandle.fptr < FatFSFileHandle.fsize - DSP_FIFO_BLOCK_BYTES) {

                FRESULT fres = m_file->read(p, DSP_FIFO_BLOCK_BYTES);

                if (fres == FR_OK) {

                    FIFO_ERROR fifo_res = output_stream.feed(DSP_FIFO_BLOCK_BYTES);

                    if (fifo_res != FIFO_ERROR_NONE) {
                        this->info.fifo_overruns++; // won't stop for an overrun, just count them
                    }
                } else if (fres != FR_DISK_ERR || this->info.status == DSP_STATUS_RUNNING) {

                    // We check again for the status because the ADC interrupt could've stopped the capture before
                    // TODO: do better error handling
                    if (FatFSFileHandle.fptr < FatFSFileHandle.fsize - DSP_FIFO_BLOCK_BYTES) {

                        this->halt(DSP_ERR_FILEREAD);
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

            if (++info.processed_blocks == 1 && on_first_block) {
                on_first_block();
            }

            // GPIOA->BSRR = GPIO_PIN_12 << 16;

        } else {
            this->info.fifo_overruns++; // won't stop for an overrun, just count them
        }
    }
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

    if (fres != FR_OK) {

        this->halt(DSP_ERR_FILEREAD);
        return false;

    } else {

        // LOG(
        //         "Info header:\nChannels:%d\nBits per sample :%u\nByte Rate:%lu\nCarrier:%llu\nFormat:%u\nSample rate:%lu\n",
        //         wi.n_channels, wi.bits_sample, wi.byte_rate, wi.carrier_freq, wi.format, wi.sample_rate)

        uint8_t decimation_factor = 1;

        if (wi.sample_rate) { // The stored file has sample rate information

            // The samples in the file are stored in a sample rate that may not be the same
            // as the sample rate currently used by the fft processor

            // Since we're using the same processing chain to both generate the fft and
            // feed the DAC, in general, we must adapt the sampling parameters to account
            // for the mismatch.

            // If we want to keep the same fft span and related parameters, the sample rate of the file (considering it relates directly with
            // the bandwidth the recorded signal) needs to be lower, otherwise we'll have to increase the fft span.
            // If the sample rate of the file (again, let's assume it's the same as its bandwidth) is lower than the bandwidth of the fft,
            // we'll have to filter the aliases of the recorded signals

            // To increase de sample rate of the recorded signal, we can interpolate and filter. Same steps as we do when filtering and decimating,
            // but in reverse.

            // If we don't want to interpolate and filter, we can adapt the fft span and parameters to match those of the recorded signal, but
            // always taking into account the lower bounds of the sampling rate for the TX chain (DAC and reconstruction filters)

            // Having said that, here I'm changing the span instead of throttling the sample rate of the file.
            config.fft.span = wi.sample_rate * USABLE_BW_FACTOR;

            // Update FFT and sample rate parameters
            fft_config(config.fft.span);

            uint32_t sf = fft::fft_params.sample_freq;
            // The maximum decimation factor can't be greater than DSP_BLOCK
            uint8_t max_decimation_factor = DSP_BLOCK;
            while (decimation_factor < max_decimation_factor && sf > wi.sample_rate) {
                decimation_factor <<= 1;
                sf >>= 1;
            }

            // TODO: Since we can't currently have a decimation factor other than a power of two, this calculation
            // can result in a frequency shift with respect to the one the signal was stored. Find a way to match it in every case
        } else {

            // Since we don't have the sample rate of the stored waveform, we'll set it to the effective sample rate of the fft processing
            wi.sample_rate = fft::fft_params.sample_freq / fft::fft_params.decimation_factor;
            wi.carrier_freq = radio::get_frequency();
            decimation_factor = fft::fft_params.decimation_factor;
        }

        this->info.direction = DSP_DIRECTION_OUT;
        this->info.bandwidth = fft::fft_params.span;
        this->info.sample_rate = wi.sample_rate;
        this->info.decimation_factor = decimation_factor;
        this->info.bits_per_sample = wi.bits_sample;
        this->info.n_channels = wi.n_channels; // I/Q
        this->info.block_size_bytes = DSP_BLOCK * 2 * 2;
        this->info.decimated_block_size = DSP_BLOCK * 2 / decimation_factor / (this->info.n_channels == 1 ? 2 : 1);
        this->info.decimated_block_size_bytes = this->info.block_size_bytes / decimation_factor / (this->info.n_channels == 1 ? 2 : 1);

        // Start media read processing timer
        HAL_TIM_Base_Start_IT(&TASKS_TIMER_HANDLE);

        // Se the fifo producer frequency
        // TODO: Check proper values for this. The frequency should be just enough to prevent underruns in the fifo
        update_timer(TASKS_TIMER_TYPEDEF, 2, TASKS_TIMER_TYPEDEF_CLOCK_HZ / 10000);

#ifdef __STM32F3xx_HAL_H
        // In STM32F3xx ,only as a POC, we will use only one DAC, so are limited to play only one channel
        this->status.n_channels = 1;
#endif

        // The signal samples will be interpolated by the current decimation factor to adapt the rate
        // to that of the FFT processing chain (which will in turn downsample them by the same factor)
        // So we write samples to the DAC at a rate equal to the desired sample rate multiplied
        // by the interpolation (->DAC) or decimation (ADC->) factor
        bool ret = radio_config({.direction = RF_DIRECTION_TX, .sample_freq = this->info.sample_rate * this->info.decimation_factor});

        // TODO: Manage gain globally. Not that easy considering in receive we'd need to normalize it and that's not easy for all modulations

        this->info.status = DSP_STATUS_RUNNING;
        if (!ret) {
            this->halt(DSP_ERR);
            return false;
        }

        // Capture/Replay won't apply frequency shifts (for DC issues mitigation) because:
        // - The ReplayProcessor is also used in other tasks (e.g. APRS trasnsmit) that do not accout for the shift
        // - Honestly I haven't take the time to think about why it would benefit from the shifting, but it is very likely
        //   required since the DC blocker is surely killing whatever is at DC (the carrier itself if any)
        dsp::enable_frequency_shift(false);
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

        dsp::enable_frequency_shift(true);

        Task::stop(); // Let the base class finish

        fft_config(config.fft.span);

        radio_config({.direction = RF_DIRECTION_RX, .sample_freq = 0, .freq = 0, .mode = DSP});

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
