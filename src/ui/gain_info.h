//
// Created by Angel Dust on 09/09/2025.
//

#ifndef TRX_FRONTEND_GAIN_INFO_WIDGET_H
#define TRX_FRONTEND_GAIN_INTO_WIDGET_H

#include "widget.h"
#include "types.h"

class GainInfoWidget : public Widget {
  public:
    GainInfoWidget(const Rect &parentRect, Display *display) : Widget(parentRect, display){};

    bool paint_callback() override;

    bool on_touch(const st_inputEvent e) override;

  protected:
    void before_paint() override;
    float s_level = 0;
};

#endif // TRX_FRONTEND_GAIN_INFO_WIDGET_H
