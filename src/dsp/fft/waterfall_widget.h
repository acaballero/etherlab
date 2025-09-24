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

    void move(int16_t);

    // Set the scrolled pixels per frame
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

    const uint32_t waterfall_palette_rgb256_dx[FFT_WATERFALL_NCOLORS] = {0x000010, 0x000040, 0x000070, 0x100090, 0x3000b0, 0x6000d0, 0x8000f0, 0xa000d0,
                                                                         0xc000b0, 0xe00090, 0xff0060, 0xff0030, 0xff0000, 0xd00000, 0xa00000, 0x600000};

    // Buffer to convert the waterfall palette from RGB888 to RGB565
    // TODO: Just create a RGB565 palette
    uint16_t waterfall_palette_rgb565[FFT_WATERFALL_NCOLORS];

    /* When the center frequency of the FFT changes, we need to scroll the waterfall accordingly. But the waterfall will scroll in
     * multiples of 2 frequency bins (for optimization) and if the frequency change is less than that, it won't move. Therefore,
     * we need to store the frequency of the waterfall to know when it's difference with the center frequency it's enough to scroll it */
    unsigned long waterfallFreq;

    int16_t bins_db[width];

    WATERFALL_MODE mode{MAX_HOLD};

    uint16_t scroll_period;

    os::periodic_task task{0, [&]() {
                               scroll();
                           }};

    /* Integrate the current spectrum values */
    void integrate();

    bool on_touch(const st_inputEvent e) override;

    uint8_t step;
};

#endif // TRX_FRONTEND_WATERFALL_WIDGET_H
