//
// Created by Angel Dust on 08/06/2024.
//

#ifndef TRX_FRONTEND_POWMETER_WIDGET_H
#define TRX_FRONTEND_POWMETER_WIDGET_H

#include "widget.h"
#include "types.h"
#include "rf_coupler.h"

class PowerMeterWidget : public Widget {
  public:
    using Widget::Widget;
    PowerMeterWidget(Rect parent_rect, Display *display);
    bool paint_callback() override;

  protected:
    void before_paint() override;

    float toWatts(float dbm);

    rf_coupler::rf_coupler_info info;

    int margin_right = margin;
};

#endif // TRX_FRONTEND_POWMETER_WIDGET_H
