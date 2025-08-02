//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_TUNE_WIDGET_H
#define TRX_FRONTEND_TUNE_WIDGET_H

#include "widget.h"
#include "types.h"

class TuneWidget : public Widget {
  public:
    TuneWidget(const Rect &parentRect, Display *display) : Widget(parentRect, display){};

    bool paint_callback() override;

    bool on_touch(const st_inputEvent e) override;

  protected:
    void before_paint() override;
    float s_level = 0;
};

#endif // TRX_FRONTEND_TUNE_WIDGET_H
