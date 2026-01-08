//
// Created by Angel Dust on 03/01/2026.
//

#ifndef TITLEBAR_ICONS_WIDGET_H
#define TITLEBAR_ICONS_WIDGET_H

#include "button_widget.h"
#include "hw/stm32f4xx/rtc.h"
#include "ui/frequency_widget.h"
#include "ui/power_metrics_widget.h"
#include "view.h"
#include "types.h"
#include "../../lib/Signal/Signal.h"

class TitleBarIconsWidget : public Widget {

  public:
    static constexpr uint8_t MARGIN = 3;
    TitleBarIconsWidget(const Rect &parentRect, Display *display);

    void on_info_changed_signal(const void *params);
    bool paint_callback() override;
    static void signal_static_callback(void *thisptr, const void *args) {
        ((TitleBarIconsWidget *)thisptr)->on_info_changed_signal(args);
    }

  protected:
    void before_paint() override;

    st_topBar status;

    st_datetime datetime{};
    // Refresh clock
    os::periodic_task task{1000, [this](void) {
                               set_dirty();
                           }};
};

#endif // TITLEBAR_ICONS_WIDGET_H
