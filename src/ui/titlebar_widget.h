//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_TITLEBAR_WIDGET_H
#define TRX_FRONTEND_TITLEBAR_WIDGET_H

#include "button_widget.h"
#include "ui/frequency_widget.h"
#include "view.h"
#include "types.h"
#include "../../lib/Signal/Signal.h"

class TitleBarWidgetInner : public Widget {

  public:
    static constexpr uint8_t MARGIN = 3;
    TitleBarWidgetInner(const Rect &parentRect, Display *display);

    void on_info_changed_signal(void *params);
    void paint_callback() override;
    static void signal_static_callback(void *thisptr, void *args) { ((TitleBarWidgetInner *)thisptr)->on_info_changed_signal(args); }

  protected:
    void before_paint() override;

    st_topBar status;

    Button btnDSP{{0, MARGIN, 100, area.box.height - MARGIN * 2}, display, ""};
};

class TitleBarWidget : public View {

  public:
    static constexpr uint8_t MARGIN = 3;
    TitleBarWidget(Rect parent_rect) : View(parent_rect) { init(); }

  protected:
    void before_paint() override;

    void init();

    Button btnDSP{{110, MARGIN, 0, area.box.height - MARGIN * 2}, display, ""};

    TitleBarWidgetInner titleBarWidgetInner{{0, MARGIN, 110, area.box.height}, &lcd};
};

#endif // TRX_FRONTEND_TITLEBAR_WIDGET_H
