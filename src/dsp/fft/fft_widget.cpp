//
// Created by Angel Dust on 18/04/2021.
//

#include "Display_afb.h"
#include "config.h"
#include "dsp/fft/fft_ui.h"
#include "fft_widget.h"
#include "fft.h"
#include "agc.h"
#include "input/inputEvent.h"
#include "ips_font.h"

FFTWidget::FFTWidget(const Rect &parentRect, Display *display, FFT_SPECTRUM_STYLE s) : Widget(parentRect, display), style{s} {}

void FFTWidget::draw_bandwidth() {
    int16_t bm_s, bm_e, bm_m;
    int16_t px_if_width = (int16_t)(radio::if_filters[radio::if_filter].bandwidth_khz * 1000 / fft_params.display_rbw) >> 1;
    bm_m = DISPLAY_X_PIXELS >> 1;

    if (config.modulation == SSB_USB) {
        bm_s = bm_m + 1;
        bm_e = bm_m + (px_if_width << 1) - 1;
    } else if (config.modulation == SSB_LSB) {
        bm_s = bm_m - (px_if_width << 1) + 1;
        bm_e = bm_m - 1;
    } else {
        bm_s = bm_m - px_if_width;
        bm_e = bm_m + px_if_width;
    }

    bm_s = bm_s < 0 ? 0 : bm_s;
    bm_e = bm_e > (FFT_ZONE_WIDTH - 1) ? (FFT_ZONE_WIDTH - 1) : bm_e;

    for (uint16_t i = bm_s; i <= bm_e; i++) {
        if (i != bm_m) {
            display->writeVertLine(i, 0, FFT_HEIGHT - 1, SWAP_BYTES(RGB888_TO_RGB565(0x333333)));
        }
    }

    display->writeVertLine(bm_m, 0, FFT_HEIGHT - 1, C565_GREY_DARK);
}

void FFTWidget::draw_freq_marks() {
    int arr_idx_freqs[10];
    uint8_t n = findFreqs(arr_idx_freqs, 10);
    st_freq_mem data;

    int text_width, padding = 3, padding_v = 3;
    FontDef *font = (FontDef *)&Font_Fixed5x7;
    display->setFont(font);
    int height = font->height + padding_v * 2 - 1;
    int margin_top = 1;

    while (n) {
        data = config.freqs[arr_idx_freqs[n - 1]];
        uint16_t x = ((float)(data.freq - fft_params.span_f_start) / (float)(fft_params.span)) * FTT_DISPLAY_WIDTH;
        text_width = strlen(data.name) * font->width;
        int x0 = x - (text_width / 2) - padding;
        int x1 = x0 + padding * 2 + text_width;
        if (x0 >= 0 && x1 < FFT_ZONE_WIDTH) {

            // The drawing zone is slightly smaller than the spectrum width to have space for the DB scale widget
            display->writeVertLine(x, margin_top + height, FFT_HEIGHT, C565_GREY_DARKER);

            display->setColor(C565_GREY_DARKER);
            display->setBgColor(C565_DARKEST);
            display->drawRoundedRectangle(x0, margin_top, text_width + padding * 2, height, 3, false);
            display->gotoXY(x - (text_width / 2), margin_top + padding_v);
            display->setColor(C565_GREY_LIGHT);
            display->write(data.name);
        }
        n--;
    }
}

bool FFTWidget::on_touch(const st_inputEvent) {
    fftUI::open_span_config();
    return true;
}

void FFTWidget::draw_span_marks() {
    char buf[10];

    uint8_t y0 = 2;
    uint16_t x2 = FFT_ZONE_WIDTH - 30;
    uint16_t span = fft_params.span / 1000 / 2;
    display->setFont((FontDef *)&Font_Fixed5x7);
    display->setColor(C565_GREY_LIGHT);
    display->setBgColor(C565_GREY_DARKER);
    display->setVerticalLineSpacing(2);
    display->fill(0, y0, 5 * 6, y0 + 9, C565_GREY_DARKER);
    display->fill(x2, y0, x2 + 5 * 6, y0 + 9, C565_GREY_DARKER);

    display->gotoXY(3, y0 + 2);
    sprintf(buf, "-%3dk", span);
    display->print(buf);
    display->gotoXY(x2 + 3, y0 + 2);
    sprintf(buf, "+%3dk", span);
    display->print(buf);
    display->setFont((FontDef *)&Font_7x10);
}

void FFTWidget::draw_h_labels() {

    char buf[6];
    int max_label_width = 5 * 5;
    int n_divs = DISPLAY_X_PIXELS / (max_label_width << 1);
    // Must be even to have one tick at the center
    if (n_divs % 2 == 1) {
        n_divs--;
    }

    uint32_t delta_khz = config.fft.span / n_divs / 1000;
    uint16_t delta_x = DISPLAY_X_PIXELS / n_divs;

    display->setFont((FontDef *)&Font_Fixed5x7);
    display->setColor(C565_GREY_LIGHT);
    display->setBgColor(C565_TRANSPARENT);

    // Start
    float f_khz = (config.vfo[config.vfo_ix].freq / 1000) - (((n_divs - 1) >> 1) * delta_khz);
    uint16_t x = (DISPLAY_X_PIXELS >> 1) - (((n_divs >> 1) - 1) * delta_x);

    for (int i = 0; i < n_divs - 1; i++) {
        float f_mhz = (float)f_khz / 1000.0f;
        if (delta_khz > 200) {
            sprintf(buf, "%.1f", f_mhz);
        } else {
            sprintf(buf, "%.2f", f_mhz);
        }
        display->gotoXY(x - (((int)strlen(buf)) * 2), FFT_HEIGHT + 3);
        display->print(buf);
        f_khz += delta_khz;
        x += delta_x;
    }
}

void FFTWidget::draw_peak() {
    if (fft_peak_bin) {
        // Peak is in bin units and need to be translated to display units

        uint16_t peak_x = fft_peak_bin / fft_params.bin_width_px;

        // If the LO is high-side injected, the bins in the fft are in reverse frequency order
        if (radio::is_freq_inverted()) {
            peak_x = DISPLAY_X_PIXELS - peak_x - 1;
        }

        if (peak_x < FFT_ZONE_WIDTH - 4) {

            // display->setColor(C565_BLACK);
            display->writeLine(max2(peak_x - 1, 0), 2, peak_x + 1, 2, C565_WHITE);
            display->writeVertLine(peak_x, 1, 3, C565_WHITE);
        }
    }
}

void FFTWidget::draw_noise_floor() {
    if (fft_calc_noise_floor_period_ms > 0) {
        uint16_t py =
            FFT_HEIGHT - (uint8_t)(((float)(fft_noise_floor_db - config.fft.min_db) / (float)(config.fft.max_db - config.fft.min_db)) * (float)FFT_HEIGHT);
        display->writeLine(0, py, FFT_ZONE_WIDTH - 1, py, C565_PINK);
    }
}

void FFTWidget::draw_spectrum_fill() {
    for (uint16_t i = 0; i < FFT_ZONE_WIDTH; i++) {
        // Although the fft_display array is wider, we will only draw the zone width to allow for the Db scale to be drawn next to it
        if (fft_display[i] < FFT_HEIGHT) {
            display->writeVertLine(i, fft_display[i], FFT_HEIGHT, spectrum_fill_color);
        }
    }
}

void FFTWidget::draw_spectrum_line() {

    for (uint16_t i = 0; i < FFT_ZONE_WIDTH - 1; i++) {
        // Although the fft_display array is wider, we will only draw the zone width to allow for the Db scale to be drawn next to it
        if (fft_display[i] < FFT_HEIGHT) {
            display->writeLine(i, fft_display[i], i + 1, fft_display[i + 1], spectrum_line_color);
        }
    }
}

void FFTWidget::draw_spectrum() {

    switch (style) {
        case FFT_SPECTRUM_STYLE_LINE:
            draw_spectrum_line();
            break;
        case FFT_SPECTRUM_STYLE_FILL:
            draw_spectrum_fill();
            break;
        case FFT_SPECTRUM_STYLE_LINE_FILL:
            draw_spectrum_fill();
            draw_spectrum_line();
            break;
    }

    display->setBgColor(C565_BLACK);

    if (fft_mag_overload) {
        display->gotoXY(10, FFT_HEIGHT - 20);
        display->setColor(C565_RED);

        display->print("ADC OVERLOAD");
    }

    if (agc::is_overload()) {
        display->gotoXY(10, FFT_HEIGHT - 20);
        display->setColor(C565_RED);
        display->print("DSP OVERLOAD");
    }
}

void FFTWidget::paint_callback() {

    display->clear();

    // DEBUG: slice mark
    // if (fft_slice_n > 0) {
    //       display->writeLine(x, 0, x, 10, C565_GRAY);
    //   }

    // uint16_t start;

    // GPIOA->BSRR= GPIO_PIN_15;

    draw_bandwidth();

    draw_freq_marks();

    draw_spectrum();

    // GPIOA->BSRR= GPIO_PIN_15 << 16;

    draw_peak();

    draw_noise_floor();

    draw_span_marks();

    if (refresh_x_axis) {
        draw_h_labels();
    }
}

void FFTWidget::before_paint() {
    if (this->dirty()) {
        refresh_x_axis = f_start != fft_params.span_f_start || fft_span != fft_params.span;
    }
}

void FFTWidget::set_style(FFT_SPECTRUM_STYLE v) { style = v; }

FFT_SPECTRUM_STYLE FFTWidget::get_style() { return style; }

void FFTWidget::set_colors(uint16_t line, uint16_t fill) {
    spectrum_line_color = line;
    spectrum_fill_color = fill;
}
