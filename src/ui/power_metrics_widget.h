//
// Created by Angel Dust on 23/12/2025.
//

#ifndef POWER_METRICS_WIDGET_H
#define POWER_METRICS_WIDGET_H

#include "label_widget.h"
#include "widget.h"
#include "types.h"
#include "rf_coupler.h"

class PowerMetricsWidget : public Widget {
  public:
    using Widget::Widget;
    PowerMetricsWidget(Rect parent_rect, Display *display);
    bool paint_callback() override;

  protected:
    void before_paint() override;

    float toWatts(float dbm);

    rf_coupler::rf_coupler_info info;
    int swr_block_size;

    Label lblFwd{};
    Label lblRev{};
    Label lblSwr{};
};

#endif // POWER_METRICS_WIDGET_H
