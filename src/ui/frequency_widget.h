//
// Created by Angel Dust on 18/04/2021.
//

#ifndef TRX_FRONTEND_FREQUENCY_WIDGET_H
#define TRX_FRONTEND_FREQUENCY_WIDGET_H

#include "label_widget.h"
#include "view.h"
#include "types.h"
#include <sys/_stdint.h>

class FrequencyWidget : public View {
  public:
    FrequencyWidget(Rect parent_rect) : View(parent_rect) { init(); }
    void paint_callback() override;

  protected:
    static constexpr uint8_t MARGIN = 3;
    st_freqInfo status;
    void do_paint() override;
    void init();
    bool on_input(const st_inputEvent event) override;

    Label lblRpt{{0, MARGIN, area.width / 4, area.height - MARGIN * 2}};
    Label lblVFO{{(area.width / 4) + 2, MARGIN, 24, area.height - MARGIN * 2}};
};

#endif // TRX_FRONTEND_FREQUENCY_WIDGET_H
