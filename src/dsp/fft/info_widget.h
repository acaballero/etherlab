//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_INFO_WIDGET_H
#define TRX_FRONTEND_INFO_WIDGET_H

#include "ui/widget.h"
#include "types.h"

class InfoWidget: public Widget {
public:

    using Widget::Widget;

    void paint_callback() override;

protected:
    void do_paint() override;
};

#endif //TRX_FRONTEND_INFO_WIDGET_H
