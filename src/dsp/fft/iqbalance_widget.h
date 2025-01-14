//
// Created by Angel Dust on 18/04/2021.
//

#ifndef TRX_FRONTEND_IQBALANCE_WIDGET_H
#define TRX_FRONTEND_IQBALANCE_WIDGET_H

#include "../../ui/widget.h"
#include "../../types.h"

class IQBalanceWidget: public Widget {
public:

    using Widget::Widget;

    void paint_callback() override;

    void before_paint() override;
};



#endif //TRX_FRONTEND_IQBALANCE_WIDGET_H
