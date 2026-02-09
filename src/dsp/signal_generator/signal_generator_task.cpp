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

bool SignalGeneratorTask::start_impl() {

    info.direction = DSP_DIRECTION_OUT;
    info.bandwidth = fft::fft_params.span;
    info.sample_rate = fft::fft_params.sample_freq;
    info.decimation_factor = fft::fft_params.decimation_factor;
    info.bits_per_sample = 16;
    info.n_channels = 2;
    info.block_size_bytes = DSP_BLOCK * 2 * 2;
    info.decimated_block_size = DSP_BLOCK * 2 / fft::fft_params.decimation_factor / (info.n_channels == 1 ? 2 : 1);
    info.decimated_block_size_bytes = info.block_size_bytes / fft::fft_params.decimation_factor / (info.n_channels == 1 ? 2 : 1);

    bool ret = radio_config({.direction = mode, .sample_freq = info.sample_rate / 4, .freq = 0, .mode = DSP});

    info.status = DSP_STATUS_RUNNING;

    if (!ret) {
        halt(DSP_ERR);
        return false;
    }

    return true;
}

void SignalGeneratorTask::stop() {
    if (info.status != DSP_STATUS_STOPPED) {

        info.status = DSP_STATUS_STOPPED;

        Task::stop(); // Let the base class finish
    }
}
