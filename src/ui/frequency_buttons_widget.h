//
// Created by Angel Dust on 03/01/2026.
//

#ifndef TRX_FRONTEND_FREQUENCY_BUTTONS_H
#define TRX_FRONTEND_FREQUENCY_BUTTONS_H

#include "Signal.h"
#include "input/inputEvent.h"
#include "label_widget.h"
#include "scanner.h"
#include "view.h"
#include "types.h"
#include <stdint.h>

class FrequencyButtonsWidget : public View {
  public:
    FrequencyButtonsWidget(Rect parent_rect) : View(parent_rect) {
        init();
    }

  protected:
    static constexpr uint8_t MARGIN = 3;
    static constexpr uint8_t LBLVFO_WIDTH = 24;
    static constexpr uint8_t LBLSCAN_WIDTH = 40;
    static constexpr uint8_t LBLRPT_WIDTH = 40;

    st_freqInfo status;
    void before_paint() override;
    void init();
    bool on_touch(const st_inputEvent) override;

    Button btnScan{{0, MARGIN, LBLSCAN_WIDTH, area.box.height - MARGIN * 2}, display, ""};
    Button btnRpt{{LBLSCAN_WIDTH + MARGIN, MARGIN, LBLRPT_WIDTH, area.box.height - MARGIN * 2}, display, ""};
    Button btnVFO{{LBLRPT_WIDTH + LBLSCAN_WIDTH + 2 * MARGIN, MARGIN, LBLVFO_WIDTH, area.box.height - MARGIN * 2}, display, ""};
};

#endif // TRX_FRONTEND_FREQUENCY_BUTTONS_H
