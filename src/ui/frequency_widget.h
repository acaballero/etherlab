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
    static constexpr uint8_t LBLSCAN_WIDTH = 50;
    static constexpr uint8_t LBLRPT_WIDTH = 50;

    uint16_t freq_xpos = LBLRPT_WIDTH + LBLVFO_WIDTH + LBLSCAN_WIDTH + MARGIN * 3;
    st_freqInfo status;
    void before_paint() override;
    void init();
    bool on_touch(const st_inputEvent) override;

    Button btnRpt{{0, MARGIN, LBLRPT_WIDTH, area.box.height - MARGIN * 2}, display, ""};
    Button btnScan{{LBLRPT_WIDTH + MARGIN, MARGIN, LBLSCAN_WIDTH, area.box.height - MARGIN * 2}, display, ""};
    Button btnVFO{{LBLRPT_WIDTH + LBLSCAN_WIDTH + 2 * MARGIN, MARGIN, LBLVFO_WIDTH, area.box.height - MARGIN * 2}, display, ""};
    FrequencyWidgetInner freqWidget{{freq_xpos, MARGIN, area.box.width - freq_xpos, area.box.height}, &lcd};
};

#endif // TRX_FRONTEND_FREQUENCY_WIDGET_H
