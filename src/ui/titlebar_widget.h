//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_TITLEBAR_WIDGET_H
#define TRX_FRONTEND_TITLEBAR_WIDGET_H

#include "widget.h"
#include "types.h"
#include "../../lib/Signal/Signal.h"

class TitleBarWidget : public Widget {

public:
    TitleBarWidget(const Rect &parentRect, Display *display);

    void paint_callback() override;

    void on_info_changed_signal(void *params);

    static void signal_static_callback(void *thisptr, void *args) {
        ((TitleBarWidget *)thisptr)->on_info_changed_signal(args);
    }

protected:

    void do_paint() override;

    st_topBar status;

};

#endif //TRX_FRONTEND_TITLEBAR_WIDGET_H
