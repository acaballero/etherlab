//
// Created by Angel Dust on 21/02/2026.
//

#ifndef TRX_FFT_X_AXIS_WIDGET_H
#define TRX_FFT_X_AXIS_WIDGET_H

#include "../../ui/widget.h"
#include "../../ui/dbscale_widget.h"
#include "../../types.h"
#include "input/inputEvent.h"
#include <vector>

#define FFT_X_AXIS_HEIGHT 12

class FFTXAxisWidget : public Widget {
  public:
    FFTXAxisWidget(const Rect &parentRect, Display *display);

    bool paint_callback() override;

  protected:
    void before_paint() override;

    void draw_h_labels();

    bool on_touch(const st_inputEvent) override;
};

#endif // TRX_FFT_X_AXIS_WIDGET_H
