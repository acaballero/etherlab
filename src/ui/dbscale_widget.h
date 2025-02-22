//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_DBSCALE_WIDGET_H
#define TRX_FRONTEND_DBSCALE_WIDGET_H

#include "input/inputEvent.h"
#include "widget.h"
#include "types.h"

#define DBSCALE_WIDTH 26

class DbScaleWidget : public Widget {
  public:
    using Widget::Widget;

    void paint_callback() override;

  protected:
    st_scale current_scale;

    void before_paint() override;
    bool on_touch(const st_inputEvent) override;
};

#endif // TRX_FRONTEND_DBSCALE_WIDGET_H
