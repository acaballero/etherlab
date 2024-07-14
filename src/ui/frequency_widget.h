//
// Created by Angel Dust on 18/04/2021.
//

#ifndef TRX_FRONTEND_FREQUENCY_WIDGET_H
#define TRX_FRONTEND_FREQUENCY_WIDGET_H

#include "widget.h"
#include "types.h"

class FrequencyWidget: public Widget {
public:

    using Widget::Widget;

    void paint_callback() override;

protected:

    st_freqInfo status;
    void do_paint() override;
    bool on_input(const st_inputEvent event) override;
};


#endif //TRX_FRONTEND_FREQUENCY_WIDGET_H
