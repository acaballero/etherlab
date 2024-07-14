//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_STATUS_WIDGET_H
#define TRX_FRONTEND_STATUS_WIDGET_H

#include "widget.h"
#include "types.h"

class StatusWidget : public Widget {
public:

    using Widget::Widget;

    void paint_callback() override;

protected:

    st_status _status;
    void print_separator();
    void do_paint() override;
};

#endif //TRX_FRONTEND_STATUS_WIDGET_H
