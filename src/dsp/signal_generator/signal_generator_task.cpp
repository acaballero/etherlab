//
// Created by Angel Dust on 02/07/2024.
//

#include "signal_generator_task.h"
#include "dsp/firFilter.h"
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

SignalGeneratorTask::SignalGeneratorTask(void (*onSucess)(), void (*onError)(DSP_ERROR)) {
    this->on_error = onError;
    this->on_success = onSucess;
}

void SignalGeneratorTask::work() {
    // This tasks does not perform any work
    // The signal is generated in the dsp processor
    // TODO: Use the FIFO as a buffer for the generated signal.
}

void SignalGeneratorTask::start() {

    this->status.direction = DSP_DIRECTION_OUT;
    this->status.bandwidth = fft_params.span;
    this->status.sample_rate = fft_params.sample_freq;
    this->status.decimation_factor = fft_params.decimation_factor;
    this->status.bits_per_sample = 16;
    this->status.n_channels = 2;
    this->status.block_size_bytes = dsp_temp_buf.size_bytes;
    this->status.decimated_block_size =
            dsp_temp_buf.count / fft_params.decimation_factor / (this->status.n_channels == 1 ? 2 : 1);
    this->status.decimated_block_size_bytes =
            this->status.block_size_bytes / fft_params.decimation_factor / (this->status.n_channels == 1 ? 2 : 1);

    bool ret = radio_config(
            {.direction=RF_DIRECTION_TX, .sample_freq=this->status.sample_rate * this->status.decimation_factor});

    this->status.status = DSP_STATUS_RUNNING;

    if (!ret) {
        this->halt(DSP_ERR);
    }

}

void SignalGeneratorTask::stop() {

    if (this->status.status != DSP_STATUS_STOPPED) {

        this->status.status = DSP_STATUS_STOPPED;

        Task::stop(); // Let the base class finish

        radio_config({RF_DIRECTION_RX, 0});
    }
}
