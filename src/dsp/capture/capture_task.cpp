//
// Created by Angel Dust on 16/04/2021.
//

#include "capture_task.h"
#include "dsp/dsp_buffers.h"
#include "stm32f4xx_hal.h"
#include "utils.hpp"

/* Should be defined in the HW abstraction layer */
extern TIM_HandleTypeDef TASKS_TIMER_HANDLE;

void CaptureTask::setFile(std::unique_ptr<File> file) {
    this->file = move(file);
}

File *CaptureTask::getFile() {
    return file.get();
}

void CaptureTask::work() {
    char *p;

    if (status.status != DSP_STATUS_RUNNING) {
        return;
    }

    // IMPORTANT: Until the SD card is integrated in the board, there are some bands (e.g. 127.1 Mhz)
    // where it radiates a lot and pollutes (heavily) the recording. There's not much to be done to prevent it apart from
    // inyecting the LO from the other side or changing IFs

    // GPIOD->BSRR |= GPIO_PIN_9;

    uint16_t av = input_stream.available(&p);
    uint32_t bytes_in = DSP_FIFO_BLOCK_BYTES;

    // PROFILE_PUSH("work");
    if (av >= bytes_in) {

        status.processed_blocks++;

        FRESULT fres = FR_OK;

        // PROFILE_PUSH("fwrite");
        fres = file->write(p, bytes_in);
        // PROFILE_POP();

        input_stream.consume(bytes_in, &p);

        if (status.status == DSP_STATUS_RUNNING) { // Maybe there was an error in the ADC thread while writing
            if (fres == FR_OK) {
                if (FatFSFileHandle.fsize > DSP_MAX_CAPTURE_SIZE) {
                    stop();
                }
            } else if (fres != FR_DISK_ERR || status.status == DSP_STATUS_RUNNING) {
                // We check again for the status because the ADC interrupt could've stopped the capture before

                halt(DSP_ERR_FILEWRITE);
            }
        }
    }
    // PROFILE_POP();

    // GPIOD->BSRR |= GPIO_PIN_9 << 16;
}

void CaptureTask::init() {

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
    dsp::set_max_sample_freq(max_sample_rate);
}

bool CaptureTask::start() {

#ifdef LCD_DISABLE_ON_DSP
    lcd.setEnabled(false);
#endif
    init();

    input_stream.reset();
    status.reset();

    this->status.direction = DSP_DIRECTION_IN;
    this->status.bandwidth = fft::fft_params.bw;
    this->status.sample_rate = config.fft.sample_rate;
    this->status.decimation_factor = fft::fft_params.decimation_factor;
    this->status.decimated_block_size = DSP_BLOCK / fft::fft_params.decimation_factor / (this->status.n_channels == 1 ? 2 : 1);
    this->status.bits_per_sample = 16;
    this->status.block_size_bytes = DSP_BLOCK * 2 * 2;
    this->status.decimated_block_size_bytes = this->status.block_size_bytes / this->status.decimation_factor / (this->status.n_channels == 1 ? 2 : 1);

    if (!lock_sd_card(5000)) {
        // prevent other tasks to use the sd_card
        this->halt(DSP_ERR);
        return false;
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

        // Update FFT and sample rate parameters
        fft_config(config.fft.span);

        radio_config({.direction = RF_DIRECTION_RX, .sample_freq = status.sample_rate, .freq = 0, .mode = DSP});

        // Start media write processing timer
        HAL_TIM_Base_Start_IT(&TASKS_TIMER_HANDLE);

        // Se the fifo consumer frequency
        update_timer(TASKS_TIMER_TYPEDEF, 2, TASKS_TIMER_TYPEDEF_CLOCK_HZ / 10000); // /10000 = N*100 microseconds

        status.status = DSP_STATUS_RUNNING;
    }

    return true;
}

void CaptureTask::stop() {

    // TODO: Flush the fifo if there are bytes left

    if (this->status.status != DSP_STATUS_STOPPED) {

        // Stop task work timer
        HAL_TIM_Base_Stop_IT(&TASKS_TIMER_HANDLE);

        if (file && file->is_open()) {
            FRESULT fres = file->close();

            if (fres != FR_OK) {
                if (this->status.error == DSP_ERR_NONE) {
                    this->status.error = DSP_ERR_FILECLOSE;
                }
            }
        }

        Task::stop(); // Let the base class do its common finish

        dsp::set_sample_freq_limits(false);

#if LCD_DISABLE_ON_DSP
        lcd.setEnabled(true);
#endif
    }

    unlock_sd_card();
}
