//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_MENU_WIDGET_H
#define TRX_FRONTEND_MENU_WIDGET_H

#include "widget.h"
#include "types.h"

class MenuWidget : public Widget {
public:

    using Widget::Widget;

    void paint_callback() override;

    bool on_input(const st_inputEvent event) override;

protected:
    void before_paint() override;
};

#endif //TRX_FRONTEND_MENU_WIDGET_H
