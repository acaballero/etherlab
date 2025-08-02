//
// Created by Angel Dust on 08/06/2024.
//

#ifndef TRX_FRONTEND_POWMETER_WIDGET_H
#define TRX_FRONTEND_POWMETER_WIDGET_H

#include "widget.h"
#include "types.h"
#include "rf_coupler.h"

#define DBM_BAR_HEIGHT 8
#define SWR_BAR_HEIGHT 3

class PowerMeterWidget : public Widget {
  public:
    using Widget::Widget;
    PowerMeterWidget(Rect parent_rect, Display *display);
    bool paint_callback() override;

  protected:
    static constexpr int margin_top = 12;
    static constexpr int margin = 20;

    void before_paint() override;
    void paint_power();
    void paint_swr();

    rf_coupler::rf_coupler_info info;
    int swr_block_size;
    int dbm_block_size;
    int dbm_nblocks;
    int dbm_tick_spacing = 5;
    int max_dbm = 40;
    int max_swr = 5;
    int margin_right = margin;
};

#endif // TRX_FRONTEND_POWMETER_WIDGET_H
