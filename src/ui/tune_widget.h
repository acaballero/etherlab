//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_TUNE_WIDGET_H
#define TRX_FRONTEND_TUNE_WIDGET_H

#include "widget.h"
#include "types.h"

class TuneWidget: public Widget {
public:

    using Widget::Widget;

    void paint_callback() override;

protected:
    void do_paint() override;
    float s_level=0;
};

#endif //TRX_FRONTEND_TUNE_WIDGET_H
