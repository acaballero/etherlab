//
// Created by Angel Dust on 04/04/2025.
//

#include "receive_task.h"
#include "arm_math.h"
#include "dsp/blocks/dc_block.h"
#include "dsp/blocks/noise_generator.h"
#include "dsp/blocks/signal_generator.h"
#include "dsp/buffer.hpp"
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
#include <cstddef>
#include <memory>
#include <sys/_stdint.h>
#include "s_strength.h"

#include "printf.h"
#include "utils.hpp"

/* Should be defined in the HW abstraction layer */
extern TIM_HandleTypeDef TASKS_TIMER_HANDLE;

//#define RECEIVETASK_DEBUG
#ifdef RECEIVETASK_DEBUG

SignalGenerator s;
NoiseGenerator n;
complex_t d[DSP_BLOCK];
buffer_t<complex_t> b{d, DSP_BLOCK};

#endif

void ReceiveTask::process_audio(buffer_t<float32_t> &buff_out_f32) {

#ifdef RECEIVETASK_DEBUG

    uint32_t freq = (HAL_GetTick() / 2) % 5000;
    int i = (HAL_GetTick() / 2 / 5000);
    int gain = -20 + i * 2;

    if (i & 1) {
        s.set_config(freq, buff_out_f32.sample_rate);
        s.set_gain_db(gain);
        s.get_block(b);
        dsp::s16_to_f32((adc_type *)b.p, buff_out_f32.p, buff_out_f32.count);
    } else {
        n.set_gain_db(gain);
        //  n.get_block(b);
    }

#endif

    if (squelch.is_noise(buff_out_f32)) {
        // Ouput silence
        memset(buff_out_f32.p, 0, buff_out_f32.size_bytes);
    } else {
        if (compressor_enabled) {
            compressor.work(buff_out_f32);
        }

        if (deemph_enabled) {
            deemph_filter.decimate(buff_out_f32, buff_out_f32, 0, 1, 1);
        } else {
            // Disabled. Minimal to negligible improvement
            //  buffer_t<float32_t> b = {(float32_t *)bi1_p, (size_t)block_size_out * 2};
            // audio_lpf.decimate(b, b, 0, 2, 2);
        }
    }
}

MODULATION_MODE ReceiveTask::get_modulation_mode() {
    return main_board::getModulationMode();
}

void ReceiveTask::set_squelch() {
    MODULATION_MODE m = get_modulation_mode();
    if ((m == FM || m == WFM) && config.squelch_level) {

        float threshold = max2(0, 10 - config.squelch_level);

        squelch.config(threshold, status.sample_rate);
        squelch_enabled = true;

    } else {
        squelch_enabled = false;
    }
}

bool ReceiveTask::init() {

    MODULATION_MODE mod = main_board::getModulationMode();

    // Init audio low-pass filter
    // audio_lpf.config(status.sample_rate, 3000, 1);

    if (dsp::dsp_config.deemphasis_enabled && (mod == FM || mod == WFM)) {
        // Init de-emphasis FM filter
        deemph_filter.config(status.sample_rate, 3000, 1, LPF);
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

    squelch_signal_token = sstrength::squelch_signal.add(NULL, [this](void *, void *) {
        set_squelch();
    });

    set_squelch();

    return true;
}

ReceiveTask::~ReceiveTask() {
    if (squelch_signal_token) {
        sstrength::squelch_signal.remove(squelch_signal_token);
    }
}
