//
// Created by Angel Dust on 18/04/2021.
//

#ifndef TRX_FRONTEND_FREQUENCY_WIDGET_H
#define TRX_FRONTEND_FREQUENCY_WIDGET_H

#include "label_widget.h"
#include "view.h"
#include "types.h"
#include <stdint.h>

class FrequencyWidgetInner : public Widget {
  public:
    FrequencyWidgetInner(Rect parent_rect, Display *display) : Widget(parent_rect, display) {}
    void paint_callback() override;

  protected:
    void before_paint() override;
};

class FrequencyWidget : public View {
  public:
    FrequencyWidget(Rect parent_rect) : View(parent_rect) { init(); }

  protected:
    static constexpr uint8_t MARGIN = 3;
    static constexpr uint8_t LBLVFO_WIDTH = 24;
    uint16_t freq_xpos = (area.box.width / 4) + LBLVFO_WIDTH + MARGIN * 3;
    st_freqInfo status;
    void before_paint() override;
    void init();
    bool on_input(const st_inputEvent event) override;

    Label lblRpt{{0, MARGIN, area.box.width / 4, area.box.height - MARGIN * 2}};
    Label lblVFO{{(area.box.width / 4) + 2, MARGIN, LBLVFO_WIDTH, area.box.height - MARGIN * 2}};
    FrequencyWidgetInner freqWidget{{freq_xpos, MARGIN, area.box.width - freq_xpos, area.box.height}, &lcd};
};

#endif // TRX_FRONTEND_FREQUENCY_WIDGET_H
