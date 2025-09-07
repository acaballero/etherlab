//
// Created by Angel Dust on 02/07/2024.
//

#include "signal_generator_task.h"
#include "dsp/fir_filter.h"
#include "dsp/dsp_common.h"
#include "status.h"
#include "hw/stm32f4xx/timers.h"
#include "config.h"
#include "FIFO.h"
#include "types.h"
#include "ui/view.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"

/* Should be defined in the HW abstraction layer */
extern TIM_HandleTypeDef TASKS_TIMER_HANDLE;

void SignalGeneratorTask::work() {
    // This tasks does not perform any work
    // The signal is generated in the dsp processor
    // TODO: Use the FIFO as a buffer for the generated signal.
}

bool SignalGeneratorTask::start() {

    this->status.direction = DSP_DIRECTION_OUT;
    this->status.bandwidth = fft::fft_params.span;
    this->status.sample_rate = fft::fft_params.sample_freq;
    this->status.decimation_factor = fft::fft_params.decimation_factor;
    this->status.bits_per_sample = 16;
    this->status.n_channels = 2;
    this->status.block_size_bytes = DSP_BLOCK * 2 * 2;
    this->status.decimated_block_size = DSP_BLOCK * 2 / fft::fft_params.decimation_factor / (this->status.n_channels == 1 ? 2 : 1);
    this->status.decimated_block_size_bytes = this->status.block_size_bytes / fft::fft_params.decimation_factor / (this->status.n_channels == 1 ? 2 : 1);

    bool ret =
        radio_config({.direction = mode,
                      .sample_freq = this->status.sample_rate * this->status.decimation_factor,
                      .freq = 0,
                      .mode = (mode == RF_DIRECTION_RX) ? DSP : ANALOG}); // Radio mode is DSP if the ouput mode is RX so the signal reaches the audio amp

    this->status.status = DSP_STATUS_RUNNING;

    if (!ret) {
        this->halt(DSP_ERR);
        return false;
    }

    return true;
}

void SignalGeneratorTask::stop() {

    if (this->status.status != DSP_STATUS_STOPPED) {

        this->status.status = DSP_STATUS_STOPPED;

        Task::stop(); // Let the base class finish

        // TODO: Centralize returning to digital RX
        radio_config({RF_DIRECTION_RX, 0});
    }
}
