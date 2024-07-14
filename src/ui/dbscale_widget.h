//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_DBSCALE_WIDGET_H
#define TRX_FRONTEND_DBSCALE_WIDGET_H

#include "widget.h"
#include "types.h"

#define DBSCALE_WIDTH 18

class DbScaleWidget: public Widget {
public:

    using Widget::Widget;

    void paint_callback() override;

protected:

    st_scale current_scale;

    void do_paint() override;
};


#endif //TRX_FRONTEND_DBSCALE_WIDGET_H
