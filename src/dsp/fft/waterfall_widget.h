//
// Created by Angel Dust on 19/04/2021.
//

#ifndef TRX_FRONTEND_WATERFALL_WIDGET_H
#define TRX_FRONTEND_WATERFALL_WIDGET_H

#include "../../ui/widget.h"
#include "../../types.h"
#include "fft_types.h"

#define FFT_WATERFALL_HEIGHT 36
#define FFT_WATERFALL_DEFAULT_COLOR_INDEX 2

class WaterfallWidget : public Widget {
public:

    WaterfallWidget(const Rect &parentRect, ILI9341 *display);

    void paint_callback() override;

    void centerSpectrum();

    void moveSpectrum(int16_t);

protected:

    void do_paint() override;

    const uint32_t waterfall_palette_rgb256[FFT_WATERFALL_NCOLORS] = {
            0x0000, 0x000085, 0x0000c8, 0x4B00e3,
            0x7000f1, 0xa71ad4, 0xb935aa, 0xca507f,
            0xdc6a55, 0xed852a, 0xffa000, 0xffbf55,
            0xffcf7f, 0xffdfaa, 0xffefd4, 0xffffff
    };

    // Buffer to convert the waterfall palette from RGB888 to RGB565
    // TODO: Just create a RGB565 palette
    uint16_t waterfall_palette_rgb565[FFT_WATERFALL_NCOLORS];

/* When the center frequency of the FFT changes, we need to scroll the waterfall accordingly. But the waterfall will scroll in
 * multiples of 2 frequency bins (for optimization) and if the frequency change is less than that, it won't move. Therefore,
 * we need to store the frequency of the waterfall to know when it's difference with the center frequency it's enough to scroll it */
    unsigned long waterfallFreq;
};

#endif //TRX_FRONTEND_WATERFALL_WIDGET_H
