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
#include "dsp/dsp.h"
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
#include "tinyusb/usb_audio_dsp_bridge.h"
#include "types.h"
#include "ui/view.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"
#include <cstddef>
#include <memory>

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

    if (squelch_enabled && squelch.is_noise(buff_out_f32)) {
        // Ouput silence
        memset(buff_out_f32.p, 0, buff_out_f32.size_bytes);
    } else {

        if (audio_bpf_enabled) {
            audio_bpf.decimate(buff_out_f32, buff_out_f32, 0, 1, 1);
        }

        if (deemph_enabled) {
            deemph_filter.decimate(buff_out_f32, buff_out_f32, 0, 1, 1);
        }

        if (compressor_enabled) {
            compressor.work(buff_out_f32);
        }
    }
}

MODULATION_MODE ReceiveTask::get_modulation_mode() const {
    return main_board::get_modulation_mode();
}

void ReceiveTask::set_squelch() {

    MODULATION_MODE m = get_modulation_mode();
    if ((m == FM || m == WFM) && config.squelch_level) {
        float threshold = max2(0, 10 - config.squelch_level);
        squelch.config(threshold, status.sample_rate, 1.4f * get_audio_bw_hz());
        squelch_enabled = true;
    } else {
        squelch_enabled = false;
    }
}

bool ReceiveTask::init() {

    MODULATION_MODE mod = main_board::get_modulation_mode();

    if (dsp::apply_audio_bpf()) {
        audio_bpf.config(status.sample_rate, get_audio_bw_hz(), 1, mod == WFM ? 30 : 300);
        audio_bpf_enabled = true;
    } else {
        audio_bpf_enabled = false;
    }

    if (dsp::apply_deemph(mod)) {
        // Init de-emphasis FM filter
        // FIXME: This is a 2nd order (12db octave). Too much slope. I has to be a 1st order (1 pole) IIR filter
        deemph_filter.config(status.sample_rate, 300, 1, LPF);
        deemph_enabled = true;
    } else {
        deemph_enabled = false;
    }

    if (dsp::apply_compression(mod)) {
        compressor_enabled = true;
        compressor.config(status.sample_rate, dsp::dsp_config.audio_compressor_threshold);
    } else {
        compressor_enabled = false;
    }

    if (!squelch_signal_token) {
        squelch_signal_token = sstrength::squelch_signal.add(NULL, [this](void *, const void *) {
            if (status.status == DSP_STATUS_RUNNING) {
                set_squelch();
            }
        });
    }

    set_squelch();

    return true;
}

ReceiveTask::~ReceiveTask() {
    if (squelch_signal_token) {
        sstrength::squelch_signal.remove(squelch_signal_token);
    }
}
