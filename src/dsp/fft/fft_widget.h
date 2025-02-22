//
// Created by Angel Dust on 18/04/2021.
//

#ifndef TRX_FRONTEND_FFT_WIDGET_H
#define TRX_FRONTEND_FFT_WIDGET_H

#include "../../ui/widget.h"
#include "../../ui/dbscale_widget.h"
#include "../../types.h"
#include "input/inputEvent.h"

#define FFT_HEIGHT 70
#define FFT_X_AXIS_HEIGHT 12
#define FFT_WIDGET_HEIGHT (FFT_HEIGHT + FFT_X_AXIS_HEIGHT)
#define FFT_ZONE_WIDTH (DISPLAY_X_PIXELS - DBSCALE_WIDTH)

class FFTWidget : public Widget {
  public:
    FFTWidget(const Rect &parentRect, Display *display, FFT_SPECTRUM_STYLE s);
    void set_style(FFT_SPECTRUM_STYLE style);
    void set_colors(uint16_t line, uint16_t fill);
    void paint_callback() override;

  protected:
    void before_paint() override;
    void draw_bandwidth();
    void draw_freq_marks();
    void draw_span_marks();
    void draw_peak();
    void draw_noise_floor();
    void draw_spectrum();
    void draw_h_labels();

    bool on_touch(const st_inputEvent) override;

    FFT_SPECTRUM_STYLE get_style();
    void draw_spectrum_fill();
    void draw_spectrum_line();

    uint64_t f_start;
    uint32_t fft_span;
    FFT_SPECTRUM_STYLE style = FFT_SPECTRUM_STYLE_LINE;
    uint16_t spectrum_line_color = C565_YELLOW;
    uint16_t spectrum_fill_color = C565_BLUE;

    bool refresh_x_axis;
};

#endif // TRX_FRONTEND_FFT_WIDGET_H
