//
// Created by Angel Dust on 19/04/2021.
//

#include "waterfall_widget.h"
#include "config.h"
#include "fft.h"

/* 4-bit per pixel, 16-color buffer */
__attribute__((section(".fccmram")))
#define PIXELS_PER_BYTE 2
uint8_t waterfallBuffer[DISPLAY_X_PIXELS * FFT_WATERFALL_HEIGHT * PIXELS_PER_BYTE];

WaterfallWidget::WaterfallWidget(const Rect &parentRect, Display *display) : Widget(parentRect, display) {

    this->display->convertPalette888to565(this->waterfall_palette_rgb256, this->waterfall_palette_rgb565, 16);

    memset(waterfallBuffer, FFT_WATERFALL_DEFAULT_COLOR_INDEX + (FFT_WATERFALL_DEFAULT_COLOR_INDEX << 4), sizeof(waterfallBuffer));

    this->waterfallFreq = radio::get_frequency();
}

void WaterfallWidget::centerSpectrum() {

    if (waterfallFreq == 0) {
        waterfallFreq = radio::get_frequency(); // initialize it
    } else {
        volatile int32_t f_offset = (int32_t)waterfallFreq - (int32_t)radio::get_frequency();

        // Calculate the equivalent width in buffer bytes
        int16_t offset_pixels = round((float)f_offset / fft_params.display_rbw / PIXELS_PER_BYTE);

        // To scroll horizontally, we 'memmove' the buffer, then erase the unwanted pixels
        // Remember there's 4-bit by pixel, so we divide the displacement by two
        // This limits the resolution of the scroll to 2 pixels per move.
        // TODO: In order to have 1 pixel per move, we should shift all the bytes in the buffer by 4 bits in the required direction

        if (offset_pixels != 0) {
            moveSpectrum(offset_pixels);
            // Update the waterfall frequency which will differ from f_carrier as we have moved it by multiples of bin_offset
            waterfallFreq -= offset_pixels * fft_params.display_rbw * PIXELS_PER_BYTE;
        }
    }
}

/*
 * Displaces the waterfall by frequency offset
 */
void WaterfallWidget::moveSpectrum(int16_t bin_offset) {

    uint16_t width = this->size().width();

    // check for overflow
    bin_offset = constrain(bin_offset, -width / PIXELS_PER_BYTE, width / PIXELS_PER_BYTE);

    void *orig = bin_offset > 0 ? waterfallBuffer : waterfallBuffer - bin_offset;
    void *dest = bin_offset > 0 ? waterfallBuffer + bin_offset : waterfallBuffer;

    memmove(dest, orig, (width * (FFT_WATERFALL_HEIGHT / PIXELS_PER_BYTE)) - bin_offset);

    // Clear the start or end of the buffer
    uint16_t xs = bin_offset > 0 ? 0 : (width / PIXELS_PER_BYTE) + bin_offset;
    uint16_t xe = xs + abs(bin_offset);

    uint8_t defByteVal = FFT_WATERFALL_DEFAULT_COLOR_INDEX + (FFT_WATERFALL_DEFAULT_COLOR_INDEX << 4);

    while (xs < xe) {
        for (uint16_t y = 0; y < FFT_WATERFALL_HEIGHT; y++) {
            waterfallBuffer[(y * (width / PIXELS_PER_BYTE)) + xs] = defByteVal;
        }
        xs++;
    }
}

void WaterfallWidget::paint_callback() {

    uint8_t colorIndex;
    uint8_t *pbyte;
    uint16_t b565_color;
    uint16_t width = this->size().width();

    display->clear();

    uint16_t *buffer = display->getBuffer();

    pbyte = waterfallBuffer +
            (this->display->current_line * (width >> 1)); // position in the buffer (we know x1 and x2 are always 0 and DISPLAY_X_PIXELS in this buffer)
    uint8_t *pend = waterfallBuffer + ((this->display->current_last_line + 1) * (width >> 1));
    uint8_t byte;

    while (pbyte != pend) {

        byte = *pbyte;

        for (uint8_t j = 0; j < PIXELS_PER_BYTE; j++) { // 2 pixels per byte

            colorIndex = byte & 0x000FU;

            byte >>= 4;

#if DEBUG_LCD
            /* Black and white are forzed to be 0x0000 and 0xFFFF even if they're not in the palette, to be able to see the debug messages */

            if (colorIndex == 1) {
                b565_color = 0xFFFF;
            } else if (colorIndex == 0) {
                b565_color = 0x0000;
            } else {
                b565_color = waterfall_palette_rgb565[colorIndex];
            }
#else
            b565_color = waterfall_palette_rgb565[colorIndex];
#endif

            // display->setPixel(x, y, b565_color); // too slow
            *(buffer++) = b565_color;
        }

        pbyte++;
    }
}

void WaterfallWidget::before_paint() {

    if (this->dirty()) {

        uint16_t width = this->size().width();

        // Scroll buffer down by a pixel. Remember there's 4-bit by pixel, so we divide the displacement by two

        uint16_t delta = width / PIXELS_PER_BYTE;
        memmove(waterfallBuffer + delta, waterfallBuffer, (width * (FFT_WATERFALL_HEIGHT / PIXELS_PER_BYTE)) - delta);

        // Set the first row of pixels
        uint16_t ix = 0;
        for (uint16_t i = 0; i < width; i++) {

            // IF we have the fft_display expressed in dB
            float db = fft_display_db[i] + 18;

            db = constrain(db, config.fft.min_db, FFT_MAX_DB);

            uint8_t color = (uint8_t)(((float)(db - config.fft.min_db) / (float)(FFT_MAX_DB - config.fft.min_db)) * (float)FFT_WATERFALL_NCOLORS - 1);

            // IF fft_display is represented in display 'Y' coordinates (not in dBs)
            // int py = fft_display[i];

            // uint8_t color = (uint8_t) (
            //         ((float) (FFT_HEIGHT - py) / (float) (FFT_HEIGHT)) *
            //         (float) (FFT_WATERFALL_NCOLORS - 1));

#if DEBUG_LCD
            if (color < 2)
                color = 2; // 0 and 1 are reserved in debug mode to black and white to allow writing debug messages in the pixel buffer
#endif
            // Set the 4 bits of the pixel in the buffer
            uint8_t shift;
            uint8_t mask;

            ix = ((0 * width) + i) >> 1;

            shift = (i % 2) << 2;

            mask = waterfallBuffer[ix] & (uint8_t) ~(0x000FU << shift);

            waterfallBuffer[ix] = mask | ((color % 16) << shift);
        }
    }
}
