//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_TITLEBAR_WIDGET_H
#define TRX_FRONTEND_TITLEBAR_WIDGET_H

#include "button_widget.h"
#include "hw/stm32f4xx/rtc.h"
#include "ui/power_metrics_widget.h"
#include "ui/titlebar_icons_widget.h"
#include "view.h"
#include "types.h"

class TitleBarWidget : public View {

  public:
    static constexpr uint8_t MARGIN = 3;
    TitleBarWidget(Rect parent_rect) : View(parent_rect) {
        init();
    }

  protected:
    void before_paint() override;

    void init();

    Button btnDSP{{120 + MARGIN, MARGIN, 0, area.box.height - MARGIN * 2}, display, ""};

    TitleBarIconsWidget titleBarWidgetInner{{0, MARGIN, 110, area.box.height - MARGIN}, display};

    // Refresh clock
    os::periodic_task task{1000, [this](void) {
                               set_dirty();
                               titleBarWidgetInner.set_dirty();
                           }};
};

#endif // TRX_FRONTEND_TITLEBAR_WIDGET_H
