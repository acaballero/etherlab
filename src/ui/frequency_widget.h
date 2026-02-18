//
// Created by Angel Dust on 18/04/2021.
//

#ifndef TRX_FRONTEND_FREQUENCY_WIDGET_H
#define TRX_FRONTEND_FREQUENCY_WIDGET_H

#include "Signal.h"
#include "input/inputEvent.h"
#include "label_widget.h"
#include "scanner.h"
#include "view.h"
#include "types.h"
#include <stdint.h>

class FrequencyWidget : public Widget {
  public:
    FrequencyWidget(Rect parent_rect, Display *display) : Widget(parent_rect, display) {
        set_name("f_in");
        set_focusable(true);
        set_active(false);
    }
    bool paint_callback() override;

  protected:
    bool changing_step = false;
    st_freqInfo status;
    void before_paint() override;
    bool on_input(st_inputEvent e) override;
    bool on_touch(const st_inputEvent e) override;
};

#endif // TRX_FRONTEND_FREQUENCY_WIDGET_H
