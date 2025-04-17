//
// Created by Angel Dust on 16/04/2021.
//

#include "capture_task.h"
#include "dsp/dsp_buffers.h"

/* Should be defined in the HW abstraction layer */
extern TIM_HandleTypeDef TASKS_TIMER_HANDLE;

CaptureTask::CaptureTask(void (*onSucess)(), void (*onError)(DSP_ERROR)) {
    this->on_error = onError;
    this->on_success = onSucess;
}

void CaptureTask::setFile(std::unique_ptr<File> file) {
    this->file = move(file);
}

File *CaptureTask::getFile() {
    return file.get();
}

void CaptureTask::work() {
    char *p;

    uint16_t av = input_stream.available(&p);

    // GPIOD->BSRR |= GPIO_PIN_6;
    if (av >= DSP_FIFO_BLOCK_BYTES) {
        while (av >= DSP_FIFO_BLOCK_BYTES) {
            this->status.processed_blocks++;

            FRESULT fres = FR_OK;

            // GPIOD->BSRR |= GPIO_PIN_7;
            fres = file->write(p, DSP_FIFO_BLOCK_BYTES);
            // GPIOD->BSRR |= GPIO_PIN_7 << 16;

            /* DEBUGPRINT("I:",0);
             for (int i=0;i<bytesWrote/2;i+=2) {
                 DEBUGPRINT("%d,",((uint16_t*)p)[i]);
             }
             DEBUGPRINT("\nQ:",0);
             for (int i=1;i<bytesWrote/2;i+=2) {
                 DEBUGPRINT("%d,",((uint16_t*)p)[i]);
             }
             DEBUGPRINT("\n",0);*/

            // Free the FIFO
            input_stream.consume(DSP_FIFO_BLOCK_BYTES, &p);

            if (this->status.status == DSP_STATUS_RUNNING) { // Maybe there was an error in the ADC thread while writing
                if (fres == FR_OK) {
                    if (FatFSFileHandle.fsize > DSP_MAX_CAPTURE_SIZE) {
                        this->stop();
                    }
                } else if (fres != FR_DISK_ERR || this->status.status == DSP_STATUS_RUNNING) {
                    // We check again for the status because the ADC interrupt could've stopped the capture before
                    // TODO:
                    this->halt(DSP_ERR_FILEWRITE);
                }
            }

            av = input_stream.available(&p);
        }
    } else {
        this->status.fifo_underruns++;
    }
    // GPIOD->BSRR |= GPIO_PIN_6 << 16;
}

void CaptureTask::configureDsp() {

#ifdef __STM32F3xx_HAL_H
    // Capture only 1 channel due to the limitations of the STM32F303
    this->status.n_channels = 1; // Only one channel (I)
#else
    // TODO: Select # of channels from the menu
    this->status.n_channels = 2;
#endif

    /* Filter parameters
     *
     * The filter parameters depend on the bandwidth we will process.
     * We are constrained by the processing power and data throughput available.
     * For example, if we are writing to the SD Card, the data rate should be, at least:
     * (sample_frequency*sample_size_in_bytes) bytes/s
     * Double that if we are storing I/Q samples
     * But we won't be able to be writing to the media all the time, since the ADC FIFO will need to
     * be processed too. Even if we use DMA for the transfers, it will compete for the memory bus with the ADC's DMA.
     * Therefore, if we have a media with Mkbps maximum data rate, we will assume a maximum throughput of
     * Mp*Mkbps, with Mp in (0,1)
     *
     * Mp will depend on the performance of al the other calculations (filtering, decimation...) but, in our
     * case, let's suppose it's 0.3
     *
     * If our SD Card has a Mkbps of 300 kB/s (with No DMA and SPI driver),
     * the maximum capture bandwidth will be 90 kB/s.
     *
     * If we are sampling with the ADCs at a higher frequency, we need to filter and decimate to bring down
     * the sample rate for the data capture FIFO
     *
     * The filter corner frequency will be half the capture BW (nyquist)
     *
     */

    // TODO: In reality, the maximum sample rate might be greater than this if we take into account the
    // decimation factor
    uint32_t max_sample_rate = (uint32_t)(SD_CARD_WRITE_MAX_KBPS * 1000 * 0.5 / this->status.n_channels);
    set_max_sample_freq(max_sample_rate);
}

bool CaptureTask::start() {

#ifdef LCD_DISABLE_ON_DSP
    lcd.setEnabled(false);
#endif

    input_stream.reset();

    this->status.direction = DSP_DIRECTION_IN;
    this->status.bandwidth = config.fft.span;
    this->status.sample_rate = config.fft.sample_rate;
    // this->status.delta_phase = (1000.0 / ((float) config.fft.sample_rate / (float) fft_params.decimation_factor)) * FAST_MATH_TABLE_SIZE;
    this->status.decimation_factor = fft_params.decimation_factor;
    this->status.decimated_block_size = dsp_temp_buf.count / fft_params.decimation_factor / (this->status.n_channels == 1 ? 2 : 1);
    this->status.bits_per_sample = 16;
    this->status.block_size_bytes = dsp_temp_buf.size_bytes;
    this->status.decimated_block_size_bytes = this->status.block_size_bytes / this->status.decimation_factor / (this->status.n_channels == 1 ? 2 : 1);

    while (!lock_sd_card()) {
        ; // prevent other tasks to use the sd_card
    }

    FRESULT fres; // Result after operations

    WaveInfo wi{FSTATUS_NONE,
                radio::get_frequency(),
                this->status.n_channels,
                this->status.sample_rate / this->status.decimation_factor,
                this->status.bits_per_sample,
                (uint32_t)(this->status.n_channels * this->status.bandwidth * (this->status.bits_per_sample) / 8)};

    fres = file->create(wi);

    if (fres != FR_OK) {
        this->halt(DSP_ERR_FILEOPEN);
        return false;
    } else {

        // DEBUGPRINT(
        //         "Writing wav:\nChannels:%d\nBits per sample :%u\nByte Rate:%lu\nCarrier:%llu\nFormat:%u\nSample rate:%lu\n",
        //         wi.n_channels, wi.bits_sample, wi.byte_rate, wi.carrier_freq, wi.format, wi.sample_rate);

        // Update FFT and sample rate parameters
        fft_config(config.fft.span);

        IIRDecimator_I.config(config.fft.sample_rate, this->status.bandwidth, this->status.decimation_factor);
        IIRDecimator_Q.config(config.fft.sample_rate, this->status.bandwidth, this->status.decimation_factor);

        // Start media write processing timer
        HAL_TIM_Base_Start_IT(&TASKS_TIMER_HANDLE);

        // Se the fifo consumer frequency
        update_timer(TASKS_TIMER_TYPEDEF, 2, TASKS_TIMER_TYPEDEF_CLOCK_HZ / 10000);

        this->status.status = DSP_STATUS_RUNNING;
    }

    return true;
}

void CaptureTask::stop() {

    // TODO: Flush the fifo if there are bytes left

    if (this->status.status != DSP_STATUS_STOPPED) {

        // Stop task work timer
        HAL_TIM_Base_Stop_IT(&TASKS_TIMER_HANDLE);

        if (file->is_open()) {
            FRESULT fres = file->close();

            if (fres != FR_OK) {
                if (this->status.error == DSP_ERR_NONE) {
                    this->status.error = DSP_ERR_FILECLOSE;
                }
            }
        }

        Task::stop(); // Let the base class do its common finish

        set_max_sample_freq(false);

        fft_config(config.fft.span);

#if LCD_DISABLE_ON_DSP
        lcd.setEnabled(true);
#endif
    }

    unlock_sd_card();
}
