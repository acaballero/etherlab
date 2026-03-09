//
// OOK (On-Off Keying) Transmit Task
//

#include "ook_task.h"
#include "dsp/dsp.h"
#include "dsp/fft/fft.h"
#include "dsp/dsp_common.h"
#include "main_board.h"
#include "status.h"
#include "hw/stm32f4xx/timers.h"
#include "config.h"
#include "types.h"

/* Should be defined in the HW abstraction layer */
extern TIM_HandleTypeDef TASKS_TIMER_HANDLE;

void OOKTask::work() {
    // Check if the processor finished playing the sequence
    auto *proc = get_ook_processor();
    if (proc && proc->is_finished()) {
        this->stop();
    }
}

bool OOKTask::start_impl() {

    main_board::set_mode(DIGITAL_TX);

    dsp_set_real_time(true);
    fft::set_max_decimation(4);
    dsp::set_min_sample_freq(DSP_AUDIO_SAMPLE_RATE * 8);

    info.direction = DSP_DIRECTION_OUT;
    info.bandwidth = fft::fft_params.span;
    info.sample_rate = fft::fft_params.sample_freq;
    info.decimation_factor = fft::fft_params.decimation_factor;
    info.bits_per_sample = 16;
    info.n_channels = 2;
    info.block_size_bytes = DSP_BLOCK * 2 * 2;
    info.decimated_block_size = DSP_BLOCK * 2 / fft::fft_params.decimation_factor;
    info.decimated_block_size_bytes = info.block_size_bytes / fft::fft_params.decimation_factor;

    bool ret = radio_config({.direction = RF_DIRECTION_TX, .sample_freq = info.sample_rate, .freq = 0, .mode = DSP});

    info.status = DSP_STATUS_RUNNING;

    if (!ret) {
        abort(DSP_ERR);
        return false;
    }

    return true;
}

void OOKTask::stop() {
    if (info.status != DSP_STATUS_STOPPED) {

        dsp_set_real_time(false);
        fft::set_max_decimation(config.fft.max_decimation_factor);
        info.status = DSP_STATUS_STOPPED;

        radio_config({.direction = RF_DIRECTION_RX, .sample_freq = info.sample_rate, .freq = 0, .mode = DSP});

        Task::stop();
    }
}
