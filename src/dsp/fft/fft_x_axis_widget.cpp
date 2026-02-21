//
// Created by Angel Dust on 21/02/2026.
//

#include "Display_afb.h"
#include "config.h"
#include "dsp/dsp.h"
#include "dsp/fft/fft_params.h"
#include "dsp/fft/fft_types.h"
#include "dsp/fft/fft_ui.h"
#include "fft_x_axis_widget.h"
#include "fft.h"
#include "agc.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "os/task_manager.h"
#include "radio.h"
#include "types.h"
#include "ui/frequency_memory_ui.h"
#include <utility>

FFTXAxisWidget::FFTXAxisWidget(const Rect &parentRect, Display *display) : Widget(parentRect, display) {

    // Update the frequencies in the range every time the frequency changes
    radio::freq_signal.add(this, [this](void *, const void *) {
        set_dirty();
    });
}

bool FFTXAxisWidget::on_touch(const st_inputEvent e) {

    if (!ISTX) { // Disabled while transmitting
        if (e.ms > LONG_PRESS_MS) {
            // Toggle between max span and configured span
            if (fft::fft_params.span == config.fft.span) {
                fft_config(FFT_MAX_SPAN);
            } else {
                fft_config(config.fft.span);
            }
        } else {
            fftUI::open_span_config();
        }
        return true;
    }

    return false;
}

void FFTXAxisWidget::draw_h_labels() {

    char buf[8];
    int max_label_width = 5 * 5;
    int n_divs = DISPLAY_X_PIXELS / (max_label_width << 1);
    // Must be even to have one tick at the center
    if (n_divs % 2 == 1) {
        n_divs--;
    }

    uint32_t delta_khz = fft::fft_params.span / n_divs / 1000;
    uint16_t delta_x = DISPLAY_X_PIXELS / n_divs;

    display->setFont((FontDef *)&Font_Fixed5x7);
    display->setColor(C565_GREY_LIGHT);
    display->setBgColor(C565_TRANSPARENT);

    // Start
    float f_khz = (config.vfo[config.vfo_ix].freq / 1000) - (((n_divs - 1) >> 1) * delta_khz);
    uint16_t x = (DISPLAY_X_PIXELS >> 1) - (((n_divs >> 1) - 1) * delta_x);

    for (int i = 0; i < n_divs - 1; i++) {
        float f_mhz = (float)f_khz / 1000.0f;
        if (delta_khz > 500) {
            sprintf(buf, "%.1f", f_mhz);
        } else if (delta_khz > 100) {
            sprintf(buf, "%.2f", f_mhz);
        } else {
            sprintf(buf, "%.3f", f_mhz);
        }
        display->gotoXY(x - (((int)strlen(buf)) * 2), 3);
        display->print(buf);
        f_khz += delta_khz;
        x += delta_x;
    }
}

bool FFTXAxisWidget::paint_callback() {

    display->clear();

    draw_h_labels();

    return true;
}

void FFTXAxisWidget::before_paint() {
}
