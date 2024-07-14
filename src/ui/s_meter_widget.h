//
// Created by Angel Dust on 08/06/2024.
//

#ifndef TRX_FRONTEND_SMETER_WIDGET_H
#define TRX_FRONTEND_SMETER_WIDGET_H


#include "widget.h"
#include "types.h"

#define MINOR_TICK_GAP 1
#define S_LEVELS 9
#define DB_LEVELS 6
#define MAX_S_LEVEL (S_LEVELS+DB_LEVELS)
#define S_METER_LINE_HEIGHT 8
#define MAJOR_TICK_GAP 1

class SMeterWidget : public Widget {
public:

    using Widget::Widget;

    void paint_callback() override;

protected:
    void do_paint() override;
    float get_s_level();

    float s_level = 0;

};

#endif //TRX_FRONTEND_SMETER_WIDGET_H
