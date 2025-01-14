//
// Created by Angel Dust on 29/09/2024.
//

#ifndef TRX_RADIO_STATUS_WIDGET_H
#define TRX_RADIO_STATUS_WIDGET_H

#include "view.h"
#include "../types.h"
#include "label_widget.h"
#include "button_widget.h"
#include "lcd.h"

class RadioStatusWidget : public View {
  public:
    RadioStatusWidget(Rect parent_rect) : View(parent_rect) { init(); }

  protected:
    st_radio_status _status;
    const uint8_t btn_height = 30;

    uint16_t fg_color, bg_color, dimm_color;

    Label lblMode{{0, 0, area.width / 2 - 1, btn_height - 1}};
    Button btnVFO{{area.width / 2 + 1, 0, area.width / 2 - 1, btn_height - 1}, &lcd, "", C565_BLACK};
    Button btnSquelch{{area.width / 2 + 1, btn_height, area.width / 2 - 1, btn_height}, &lcd, "", C565_BLACK};
    Button btnGain{{0, btn_height, area.width / 2 - 1, btn_height}, &lcd, "", C565_BLACK};

    char buf[20];

    void before_paint() override;

    void init();

    char *mode();

    char *squelch();

    char *gain();

    char *vfo();
};

#endif // TRX_RADIO_STATUS_WIDGET_H
