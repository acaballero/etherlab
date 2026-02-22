//
// Created by Angel Dust on 19/04/2021.
//

#ifndef TRX_FRONTEND_WATERFALL_WIDGET_H
#define TRX_FRONTEND_WATERFALL_WIDGET_H

#include "../../ui/widget.h"
#include "../../types.h"
#include "Display_afb.h"
#include "fft_types.h"
#include "input/inputEvent.h"
#include "os/periodic_task.h"
#include <stdint.h>

#define FFT_WATERFALL_HEIGHT 90
#define FFT_WATERFALL_DEFAULT_COLOR_INDEX 1

class WaterfallWidget : public Widget {
  public:
    WaterfallWidget(const Rect &parentRect, Display *display);

    bool paint_callback() override;

    void set_scroll_period(uint16_t);

    void center();

    /*
     * Moves the waterfall buffer by a number of pixels
     */
    void move(int16_t offset_px);

    /*
     * Set the scrolled pixels per frame
     */
    void set_step(uint8_t);

    void work();

    void reset();

    void scroll();

    void set_mode(WATERFALL_MODE m) {
        mode = m;
    }

  protected:
    static constexpr size_t width = DISPLAY_X_PIXELS;

    void before_paint() override;

    //    const uint32_t waterfall_palette_rgb256[FFT_WATERFALL_NCOLORS] = {0x000066, 0x000085, 0x0000c8, 0x4B00e3, 0x7000f1, 0xa71ad4, 0xb935aa, 0xca507f,
    //                                                                    0xdc6a55, 0xed852a, 0xffa000, 0xffbf55, 0xffcf7f, 0xffdfaa, 0xffefd4, 0xffffff};

    // const uint32_t waterfall_palette_rgb256_debug[FFT_WATERFALL_NCOLORS] = {0x000000, 0xffffff, 0x0000c8, 0x4B00e3, 0x7000f1, 0xa71ad4, 0xb935aa, 0xca507f,
    //                                                                       0xdc6a55, 0xed852a, 0xffa000, 0xffbf55, 0xffcf7f, 0xffdfaa, 0xffefd4, 0xffffff};

    const uint32_t waterfall_palette_rgb256_dx[FFT_WATERFALL_NCOLORS] = {
        0x000008, // 0  very deep blue-black
        0x000020, // 1  dark blue
        0x000060, // 2  brighter blue (big jump)
        0x1000A0, // 3  indigo
        0x4000D0, // 4  violet
        0x7000E0, // 5  purple-magenta
        0xA000C0, // 6  magenta
        0xD00090, // 7  pink-red
        0xF00060, // 8  red
        0xFF3030, // 9  red-orange
        0xFF6000, // 10 orange
        0xFF9000, // 11 bright orange
        0xFFB000, // 12 yellow-orange
        0xFFD000, // 13 yellow
        0xFFE080, // 14 pale yellow
        0xFFFFFF  // 15 white (compressed highlight)};
    };            // Buffer to convert the waterfall palette from RGB888 to RGB565

    // TODO: Just create a RGB565 palette
    uint16_t waterfall_palette_rgb565[FFT_WATERFALL_NCOLORS];

    /* When the center frequency of the FFT changes, we need to scroll the waterfall accordingly. But the waterfall will scroll in
     * multiples of 2 frequency bins (for optimization) and if the frequency change is less than that, it won't move. Therefore,
     * we need to store the frequency of the waterfall to know when it's difference with the center frequency it's enough to scroll it */
    uint32_t waterfall_freq;

    int16_t bins_db[width];

    WATERFALL_MODE mode{MAX_HOLD};

    bool dither = true;

    uint16_t scroll_period;

    os::periodic_task task{0, [&]() {
                               scroll();
                           }};

    /* Integrate the current spectrum values */
    void integrate();

    /*
     * Shifts the buffer one pixel to the left or right
     */
    void shift_nibbles(int8_t direction);

    bool on_touch(const st_inputEvent e) override;

    uint8_t step;

    // Top line in the buffer
    uint16_t top_y{0};
};

#endif // TRX_FRONTEND_WATERFALL_WIDGET_H
