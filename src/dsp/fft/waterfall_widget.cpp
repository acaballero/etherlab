//
// Created by Angel Dust on 19/04/2021.
//

#include "waterfall_widget.h"
#include "Display_afb.h"
#include "config.h"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft_types.h"
#include "dsp/fft/fft_ui.h"
#include "fft.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include <sys/_stdint.h>

#define PIXELS_BYTE 2
// Map db to color linearly
#define WATERFALL_LINEAR true

/* 4-bit per pixel, 16-color buffer */
CCM_SECTION uint8_t waterfallBuffer[DISPLAY_X_PIXELS * FFT_WATERFALL_HEIGHT / PIXELS_BYTE];

WaterfallWidget::WaterfallWidget(const Rect &parentRect, Display *display) : Widget(parentRect, display) {

    this->display->convertPalette888to565(this->show_fps ? this->waterfall_palette_rgb256_dx : this->waterfall_palette_rgb256_dx,
                                          this->waterfall_palette_rgb565, 16);

    memset(waterfallBuffer, FFT_WATERFALL_DEFAULT_COLOR_INDEX + (FFT_WATERFALL_DEFAULT_COLOR_INDEX << 4), sizeof(waterfallBuffer));

    this->waterfall_freq = radio::get_frequency();

    reset();
}

bool WaterfallWidget::on_touch(const st_inputEvent) {
    fftUI::open_waterfall_config();
    return true;
}

void WaterfallWidget::set_step(uint8_t value) {
    step = value;
}

void WaterfallWidget::set_scroll_period(uint16_t value) {
    scroll_period = value;
    task.set_period(value);
}

void WaterfallWidget::work() {
    integrate();
    task.run();
}

void WaterfallWidget::center() {

    if (waterfall_freq == 0) {
        waterfall_freq = radio::get_frequency(); // initialize it
    } else {
        int32_t current_freq = (int32_t)radio::get_frequency();
        int32_t f_offset = (int32_t)waterfall_freq - current_freq;

        // Calculate the equivalent width in display pixels

        int32_t offset_px = round((float32_t)f_offset / fft::fft_params.display_rbw);
        if (offset_px != 0) {
            move(offset_px);
            // Update the waterfall frequency which WILL DIFFER from current_freq since we have moved it by multiples of bin_offset
            waterfall_freq -= (int32_t)((float32_t)offset_px * fft::fft_params.display_rbw);
        }
    }
}

void WaterfallWidget::shift_nibbles(int8_t direction) {
    // direction: +1 = right, -1 = left
    uint8_t fill = FFT_WATERFALL_DEFAULT_COLOR_INDEX & 0x0F;
    for (uint16_t y = 0; y < FFT_WATERFALL_HEIGHT; y++) {
        uint8_t *row = waterfallBuffer + y * (width >> 1);
        uint8_t carry = fill;
        if (direction > 0) {
            for (uint16_t x = 0; x < (width >> 1); x++) {
                uint8_t b = row[x];
                row[x] = (b << 4) | carry;
                carry = b >> 4;
            }
        } else {
            for (int16_t x = (width >> 1) - 1; x >= 0; x--) {
                uint8_t b = row[x];
                row[x] = (carry << 4) | (b >> 4);
                carry = b & 0x0F;
            }
        }
    }
}

void WaterfallWidget::move(int16_t offset_px) {

    // To scroll horizontally, we 'memmove' the buffer, then erase the unwanted pixels

    uint16_t width = this->size().width();

    // check for overflow
    offset_px = constrain(offset_px, -width, width);

    // Remember there's 4-bit by pixel, so we divide the displacement by two to get the whole bytes, then shift by 4 if the offset is odd
    int16_t offset_bytes = offset_px / 2; // even part (btw, don't make the mistake of doing >>1 since offset_px can be negative)
    int16_t nibble_shift = offset_px & 1; // remaining odd pixel

    if (offset_bytes != 0) {
        void *orig = offset_bytes > 0 ? waterfallBuffer : waterfallBuffer - offset_bytes;
        void *dest = offset_bytes > 0 ? waterfallBuffer + offset_bytes : waterfallBuffer;

        memmove(dest, orig, (width * (FFT_WATERFALL_HEIGHT / PIXELS_BYTE)) - abs(offset_bytes));

        // Clear the start or end of the buffer
        uint16_t xs = offset_bytes > 0 ? 0 : (width / PIXELS_BYTE) + offset_bytes;
        uint16_t xe = xs + abs(offset_bytes);

        uint8_t defByteVal = FFT_WATERFALL_DEFAULT_COLOR_INDEX + (FFT_WATERFALL_DEFAULT_COLOR_INDEX << 4);

        while (xs < xe) {
            for (uint16_t y = 0; y < FFT_WATERFALL_HEIGHT; y++) {
                waterfallBuffer[(y * (width / PIXELS_BYTE)) + xs] = defByteVal;
            }
            xs++;
        }
    }

    if (nibble_shift) {
        shift_nibbles(offset_px > 0 ? 1 : -1);
    }
}

void WaterfallWidget::reset() {

    for (size_t i = 0; i < width; i++) {
        bins_db[i] = FFT_MIN_DB;
    }
}

bool WaterfallWidget::paint_callback() {

    uint8_t colorIndex;

    uint16_t b565_color;
    uint16_t width = this->size().width();

    // WARNING: This widget uses RAW BUFFER WRITES and makes a lot of bad things for the sake of performace
    uint16_t *buffer = display->getBuffer();
    uint16_t buffer_width = display->curr_area->box.width;
    int16_t ox = display->get_offset().x;
    int16_t oy = display->get_offset().y;

    // Last line of the waterfall display buffer to display in this paint iteration
    int16_t buffer_height = min2(this->size().height() + oy - 1, this->display->current_last_line) - this->display->current_line + 1;

    uint16_t delta = buffer_width > width ? buffer_width - width : 0; // container buffer bigger than ours

    const int16_t y0 = ((int32_t)this->display->current_line - oy);

    if (y0 < 0) {
        return true;
    }

    uint16_t line = (uint16_t)(top_y + (uint16_t)y0);
    if (line >= FFT_WATERFALL_HEIGHT) {
        line -= FFT_WATERFALL_HEIGHT;
    }

    uint8_t byte;

    buffer += delta ? ox : 0;

    for (int y = 0; y < buffer_height; y++) {

        const uint8_t *pbyte = waterfallBuffer + (line * width >> 1);

        for (int x = 0, px = 0; x < (width >> 1); x++, px += 2) {

            if (!delta || (px >= -ox && px < -ox + buffer_width)) {
                byte = *pbyte;

                for (uint8_t j = 0; j < PIXELS_BYTE; j++) {

                    colorIndex = byte & 0x000FU;

                    byte >>= 4;

                    b565_color = waterfall_palette_rgb565[colorIndex];

                    *(buffer++) = b565_color;
                }
            }

            pbyte++;
        }

        buffer += delta;

        line++;
        if (line >= FFT_WATERFALL_HEIGHT) {
            line = 0;
        }
    }

    return true;
}

void WaterfallWidget::integrate() {
    int min = config.fft.min_db;
    int max = config.fft.max_db;
    for (uint16_t i = 0; i < width; i++) {
        float db = fft_display_db[i];
        db = constrain(db, min, max);

        if (mode == AVERAGE) {
            bins_db[i] = bins_db[i] - 0.8f * (bins_db[i] - db);
        } else {
            if (db > bins_db[i]) {
                bins_db[i] = db;
            }
        }
    }
}

void WaterfallWidget::before_paint() {
}

void WaterfallWidget::scroll() {

    top_y = top_y + FFT_WATERFALL_HEIGHT - step;
    int max_ix = FFT_WATERFALL_NCOLORS - 1;
    if (top_y >= FFT_WATERFALL_HEIGHT) {
        top_y -= FFT_WATERFALL_HEIGHT;
    }

    int min = config.fft.min_db;
    int max = config.fft.max_db - 20;

    float range_inv = 1.0 / (max - min); // Precompute division

    uint16_t ix = 0;
    uint8_t color = 0;
    for (uint16_t i = 0; i < width; i++) {

        float db = bins_db[i];

        db = constrain(db, min, max);

        float color_f;
        float knee = fft::fft_noise_floor_db + 5;
        if (db < knee) {
            color_f = 0;
        } else {
            color_f = constrain((((float)(db - knee) * range_inv) * (float)(max_ix)) + 1, 1, max_ix);
        }
        int c = (uint8_t)color_f;

        if (show_fps && c < 2) {
            c = 2; // 0 and 1 are reserved in debug mode to black and white to allow writing debug messages in the pixel buffer
        }

        // --- Ditherhing
        if (dither) {
            static const uint8_t bayer2x2[2][2] = {{0, 128}, {192, 64}};
            static int current_line;

            current_line = !current_line;
            int tx = i & 1;            // pixel X within matrix
            int ty = current_line & 1; // pixel Y within matrix
            float frac = color_f - c;
            if (frac * 256 > bayer2x2[ty][tx] && c < 15) {
                color = c + 1;
            } else {
                color = c;
            }
        } else {
            color = c;
        }

        // Set the 4 bits of the pixel in the buffer
        uint8_t shift;
        uint8_t mask;

        ix = i >> 1;

        shift = (i % 2) << 2;

        uint8_t *row = waterfallBuffer + (top_y * width >> 1);

        mask = *(row + ix) & (uint8_t) ~(0x000FU << shift);

        *(row + ix) = mask | ((color % 16) << shift);

        for (int n = 1; n < step; n++) { // repeat as many lines as the step size
            *(row + (ix + (n * (width >> 1)))) = *(row + ix);
        }
    }

    if (mode == MAX_HOLD) {
        reset();
    }

    set_dirty();
}
