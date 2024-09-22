//
// Created by Angel Dust on 08/06/2024.
//

#ifndef TRX_FRONTEND_POWMETER_WIDGET_H
#define TRX_FRONTEND_POWMETER_WIDGET_H

#include "widget.h"
#include "types.h"
#include "rf_coupler.h"

#define P_METER_LINE_HEIGHT 10
#define DBM_BAR_HEIGHT 8
#define SWR_BAR_HEIGHT 3
#define MARGIN 9

class PowerMeterWidget : public Widget {
public:

    using Widget::Widget;
    PowerMeterWidget(Rect parent_rect, Display *display);
    void paint_callback() override;

protected:
    void do_paint() override;
    void paint_power();
    void paint_swr();

    rf_coupler::rf_coupler_info info;
    int swr_block_size;
    int dbm_block_size;
    int dbm_nblocks;
    int dbm_tick_spacing = 5;
    int max_dbm=40;
    int max_swr=5;
};

#endif //TRX_FRONTEND_POWMETER_WIDGET_H
