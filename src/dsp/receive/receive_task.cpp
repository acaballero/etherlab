//
// Created by Angel Dust on 04/04/2025.
//

#include "receive_task.h"
#include "arm_math.h"
#include "dsp/blocks/dc_block.h"
#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "dsp/decimation/dsp_fir_decimator_float_complex.h"
#include "dsp/dsp_buffers.h"
#include "dsp/fft/fft.h"
#include "dsp/fft/fft_types.h"
#include "dsp/fir_filter.h"
#include "dsp/dsp_common.h"
#include "dsp/modulation/dsp_demodulate.h"
#include "main_board.h"
#include "radio.h"
#include "status.h"
#include "hw/stm32f4xx/timers.h"
#include "config.h"
#include "FIFO.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "types.h"
#include "ui/view.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include "dsp/decimation/dsp_decimators.h"
#include <cstddef>
#include <memory>

#include "printf.h"
#include "utils.hpp"

/* Should be defined in the HW abstraction layer */
extern TIM_HandleTypeDef TASKS_TIMER_HANDLE;

void ReceiveTask::process_audio(buffer_t<float32_t> &buff_out_f32) {

    if (compressor_enabled) {
        compressor.work(buff_out_f32);
    }
    if (deemph_enabled) {
        deemph_filter.decimate(buff_out_f32, buff_out_f32, 0, 2, 2);
    } else {
        // Disabled. Minimal to negligible improvement
        //  buffer_t<float32_t> b = {(float32_t *)bi1_p, (size_t)block_size_out * 2};
        // audio_lpf.decimate(b, b, 0, 2, 2);
    }
}

MODULATION_MODE ReceiveTask::get_modulation_mode() {
    return main_board::getModulationMode();
}

bool ReceiveTask::init() {

    MODULATION_MODE mod = main_board::getModulationMode();

    // Init audio low-pass filter
    // audio_lpf.config(status.sample_rate, 3000, 1);

    if (mod == FM || mod == WFM) {
        // Init de-emphasis FM filter
        deemph_filter.config(status.sample_rate, 1500, 1);
        deemph_enabled = true;
    } else {
        deemph_enabled = false;
    }

    if (dsp::dsp_config.audio_compressor_enabled && (mod == AM || mod == SSB_USB || mod == SSB_LSB)) {
        compressor_enabled = true;
        compressor.config(status.sample_rate, dsp::dsp_config.audio_compressor_threshold);
    } else {
        compressor_enabled = false;
    }

    return true;
}
