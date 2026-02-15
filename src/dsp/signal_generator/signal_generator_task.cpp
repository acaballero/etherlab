//
// Created by Angel Dust on 02/07/2024.
//

#include "signal_generator_task.h"
#include "dsp/dsp.h"
#include "dsp/fft/fft.h"
#include "dsp/fir_filter.h"
#include "dsp/dsp_common.h"
#include "main_board.h"
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

    main_board::set_mode(mode == RF_DIRECTION_RX ? DIGITAL_RX : DIGITAL_TX);
    // Config DSP and FFT rates
    dsp_set_real_time(true);
    fft::set_max_decimation(4);                          // Enough to focus on a small bandwidth
    dsp::set_min_sample_freq(DSP_AUDIO_SAMPLE_RATE * 8); // High enough for the images to fall out of the post-DAC LPF passband

    info.direction = DSP_DIRECTION_OUT;
    info.bandwidth = fft::fft_params.span;
    info.sample_rate = fft::fft_params.sample_freq;
    info.decimation_factor = fft::fft_params.decimation_factor;
    info.bits_per_sample = 16;
    info.n_channels = 2;
    info.block_size_bytes = DSP_BLOCK * 2 * 2;
    info.decimated_block_size = DSP_BLOCK * 2 / fft::fft_params.decimation_factor;
    info.decimated_block_size_bytes = info.block_size_bytes / fft::fft_params.decimation_factor;

    bool ret = radio_config({.direction = mode, .sample_freq = info.sample_rate, .freq = 0, .mode = DSP});

    info.status = DSP_STATUS_RUNNING;

    if (!ret) {
        abort(DSP_ERR);
        return false;
    }

    return true;
}

void SignalGeneratorTask::stop() {
    if (info.status != DSP_STATUS_STOPPED) {

        dsp_set_real_time(false);
        fft::set_max_decimation(config.fft.max_decimation_factor);
        info.status = DSP_STATUS_STOPPED;

        radio_config({.direction = RF_DIRECTION_RX, .sample_freq = info.sample_rate, .freq = 0, .mode = DSP});

        Task::stop(); // Let the base class finish
    }
}
