//
// Created by Angel Dust on 19/04/2021.
//

#include "waterfall_widget.h"
#include "config.h"
#include "dsp/fft/fft_types.h"
#include "dsp/fft/fft_ui.h"
#include "fft.h"
#include "input/inputEvent.h"

#define PIXELS_BYTE 2
// Map db to color linearly
#define WATERFALL_LINEAR true

/* 4-bit per pixel, 16-color buffer */
__attribute__((section(".fccmram"))) uint8_t waterfallBuffer[DISPLAY_X_PIXELS * FFT_WATERFALL_HEIGHT / PIXELS_BYTE];

WaterfallWidget::WaterfallWidget(const Rect &parentRect, Display *display) : Widget(parentRect, display) {

    this->display->convertPalette888to565(this->show_fps ? this->waterfall_palette_rgb256_debug : this->waterfall_palette_rgb256,
                                          this->waterfall_palette_rgb565, 16);

    memset(waterfallBuffer, FFT_WATERFALL_DEFAULT_COLOR_INDEX + (FFT_WATERFALL_DEFAULT_COLOR_INDEX << 4), sizeof(waterfallBuffer));

    this->waterfallFreq = radio::get_frequency();
}

void WaterfallWidget::centerSpectrum() {

    if (waterfallFreq == 0) {
        waterfallFreq = radio::get_frequency(); // initialize it
    } else {
        int32_t f_offset = (int32_t)waterfallFreq - (int32_t)radio::get_frequency();

        // Calculate the equivalent width in buffer bytes
        int16_t offset_pixels = round((float)f_offset / fft_params.display_rbw / PIXELS_BYTE);

        // To scroll horizontally, we 'memmove' the buffer, then erase the unwanted pixels
        // Remember there's 4-bit by pixel, so we divide the displacement by two
        // This limits the resolution of the scroll to 2 pixels per move.
        // TODO: In order to have 1 pixel per move, we should shift all the bytes in the buffer by 4 bits in the required direction

        if (offset_pixels != 0) {
            moveSpectrum(offset_pixels);
            // Update the waterfall frequency which will differ from f_carrier as we have moved it by multiples of bin_offset
            waterfallFreq -= offset_pixels * fft_params.display_rbw * PIXELS_BYTE;
        }
    }
}

bool WaterfallWidget::on_touch(const st_inputEvent) {
    fftUI::open_waterfall_config();
    return true;
}

void WaterfallWidget::set_step(uint8_t value) {
    step = value;
}

/*
 * Displaces the waterfall by frequency offset
 */
void WaterfallWidget::moveSpectrum(int16_t bin_offset) {

    uint16_t width = this->size().width();

    // check for overflow
    bin_offset = constrain(bin_offset, -width / PIXELS_BYTE, width / PIXELS_BYTE);

    void *orig = bin_offset > 0 ? waterfallBuffer : waterfallBuffer - bin_offset;
    void *dest = bin_offset > 0 ? waterfallBuffer + bin_offset : waterfallBuffer;

    memmove(dest, orig, (width * (FFT_WATERFALL_HEIGHT / PIXELS_BYTE)) - bin_offset);

    // Clear the start or end of the buffer
    uint16_t xs = bin_offset > 0 ? 0 : (width / PIXELS_BYTE) + bin_offset;
    uint16_t xe = xs + abs(bin_offset);

    uint8_t defByteVal = FFT_WATERFALL_DEFAULT_COLOR_INDEX + (FFT_WATERFALL_DEFAULT_COLOR_INDEX << 4);

    while (xs < xe) {
        for (uint16_t y = 0; y < FFT_WATERFALL_HEIGHT; y++) {
            waterfallBuffer[(y * (width / PIXELS_BYTE)) + xs] = defByteVal;
        }
        xs++;
    }
}

bool WaterfallWidget::paint_callback() {

    uint8_t colorIndex;
    uint8_t *pbyte;
    uint16_t b565_color;
    uint16_t width = this->size().width();

    display->clear();

    // WARNING: This widget uses RAW BUFFER WRITES and makes a lot of bad things for the sake of performace
    uint16_t *buffer = display->getBuffer();
    uint16_t buffer_width = display->curr_area->box.width;
    int16_t ox = display->getOffset().x;
    int16_t oy = display->getOffset().y;

    // Last line of the waterfall display buffer to display in this paint iteration
    int16_t buffer_height = min2(this->size().height() + oy - 1, this->display->current_last_line) - this->display->current_line + 1;

    uint16_t delta = buffer_width - width;

    pbyte = waterfallBuffer + ((this->display->current_line - oy) * (width >> 1));

    uint8_t byte;

    buffer += ox;

    for (int y = 0; y < buffer_height; y++) {

        for (int x = 0; x < (width >> 1); x++) {

            byte = *pbyte;

            for (uint8_t j = 0; j < PIXELS_BYTE; j++) {

                colorIndex = byte & 0x000FU;

                byte >>= 4;

                b565_color = waterfall_palette_rgb565[colorIndex];

                *(buffer++) = b565_color;
            }

            pbyte++;
        }

        buffer += delta;
    }

    return true;
}

#if WATERFALL_LINEAR
// Linear projection version
void WaterfallWidget::before_paint() {

    if (this->dirty()) {
        uint16_t width = this->size().width();

        // Scroll buffer down by a pixel. Remember there's 4-bit by pixel, so we divide the displacement by log2(bits per pixels) = PIXELS_BYTE

        uint16_t delta = step * width / PIXELS_BYTE;
        memmove(waterfallBuffer + delta, waterfallBuffer, (width * (FFT_WATERFALL_HEIGHT / PIXELS_BYTE)) - delta);

        int min = config.fft.min_db;
        int max = config.fft.max_db;

        float range_inv = 1.0 / (max - min); // Precompute division

        // Set the first row of pixels
        uint16_t ix = 0;
        for (uint16_t i = 0; i < width; i++) {

            float db = fft_display_db[i] + 8;

            db = constrain(db, min, max);

            uint8_t color = (uint8_t)(((float)(db - min) * range_inv) * (float)FFT_WATERFALL_NCOLORS - 1);

            if (show_fps && color < 2) {
                color = 2; // 0 and 1 are reserved in debug mode to black and white to allow writing debug messages in the pixel buffer
            }
            // Set the 4 bits of the pixel in the buffer
            uint8_t shift;
            uint8_t mask;

            ix = ((0 * width) + i) >> 1;

            shift = (i % 2) << 2;

            mask = waterfallBuffer[ix] & (uint8_t) ~(0x000FU << shift);

            waterfallBuffer[ix] = mask | ((color % 16) << shift);
            for (int n = 1; n < step; n++) { // repeat as many lines as the step size
                waterfallBuffer[ix + (n * (width >> 1))] = waterfallBuffer[ix];
            }
        }
    }
}
#else
// Log projection version
void WaterfallWidget::before_paint() {

    if (this->dirty()) {

        uint16_t width = this->size().width();

        // Scroll buffer down by a pixel. Remember there's 4-bit by pixel, so we divide the displacement by log2(bits per pixels) = PIXELS_BYTE

        uint16_t delta = step * width / PIXELS_BYTE;
        memmove(waterfallBuffer + delta, waterfallBuffer, (width * (FFT_WATERFALL_HEIGHT / PIXELS_BYTE)) - delta);

        int min = config.fft.min_db;
        int max = FFT_MAX_DB;

        float range_inv = 1.0 / (max - min); // Precompute division

        uint8_t scale_factor = 9;

        // Set the first row of pixels
        uint16_t ix = 0;
        for (uint16_t i = 0; i < width; i++) {

            float db = constrain(fft_display_db[i], min, max);
            float normalized = (db - min) * range_inv;

            // log10 fast approximation
            float logScaled = (normalized * (scale_factor - 1)) / (1 + (scale_factor - 1) * normalized);

            int color = (int)(logScaled * (FFT_WATERFALL_NCOLORS - 1) + 0.5); // Fast rounding

            if (show_fps && color < 2) {
                color = 2; // 0 and 1 are reserved in debug mode to black and white to allow writing debug messages in the pixel buffer
            }
            // Set the 4 bits of the pixel in the buffer
            uint8_t shift;
            uint8_t mask;

            ix = ((0 * width) + i) >> 1;

            shift = (i % 2) << 2;

            mask = waterfallBuffer[ix] & (uint8_t) ~(0x000FU << shift);

            waterfallBuffer[ix] = mask | ((color % 16) << shift);
            for (int n = 1; n < step; n++) { // repeat as many lines as the step size
                waterfallBuffer[ix + (n * (width >> 1))] = waterfallBuffer[ix];
            }
        }
    }
}
#endif
