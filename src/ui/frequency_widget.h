//
// Created by Angel Dust on 18/04/2021.
//

#ifndef TRX_FRONTEND_FREQUENCY_WIDGET_H
#define TRX_FRONTEND_FREQUENCY_WIDGET_H

#include "label_widget.h"
#include "view.h"
#include "types.h"

class FrequencyWidget : public View {
  public:
    FrequencyWidget(Rect parent_rect) : View(parent_rect) { init(); }
    void paint_callback() override;

  protected:
    st_freqInfo status;
    void do_paint() override;
    void init();
    bool on_input(const st_inputEvent event) override;

    Label lblRpt{{0, 1, area.width / 4, area.height - 2}};
    Label lblVFO{{(area.width / 4) + 2, 1, 24, area.height - 2}};
};

#endif // TRX_FRONTEND_FREQUENCY_WIDGET_H
