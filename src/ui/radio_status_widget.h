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
    constexpr static uint8_t btn_height = 35;
    constexpr static uint8_t padding_top = 5;

    uint16_t fg_color, bg_color, dimm_color;

    Label lblMode{{0, padding_top, area.box.width / 2 - 1, btn_height - 1}};

    Button btnVFO{{area.box.width / 2 + 1, padding_top, area.box.width / 2 - 1, btn_height - 1}, &lcd, "", C565_BLACK};
    Button btnSquelch{{area.box.width / 2 + 1, btn_height + padding_top, area.box.width / 2 - 1, btn_height}, &lcd, "", C565_BLACK};
    Button btnGain{{0, btn_height + padding_top, area.box.width / 2 - 1, btn_height}, &lcd, "", C565_BLACK};

    char buf[20];

    void before_paint() override;

    void init();

    char *mode();

    char *squelch();

    char *gain();

    char *vfo();

    void on_button(Button &button);
};

#endif // TRX_RADIO_STATUS_WIDGET_H
