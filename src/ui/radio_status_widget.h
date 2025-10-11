//
// Created by Angel Dust on 29/09/2024.
//

#ifndef TRX_RADIO_STATUS_WIDGET_H
#define TRX_RADIO_STATUS_WIDGET_H

#include "input/inputEvent.h"
#include "view.h"
#include "../types.h"
#include "label_widget.h"
#include "button_widget.h"
#include "lcd.h"

class RadioStatusWidget : public View {
  public:
    RadioStatusWidget(Rect parent_rect) : View(parent_rect) {
        init();
    }

  protected:
    st_radio_status _status;
    constexpr static uint8_t btn_height = 38;
    constexpr static uint8_t padding_top = 6;
    constexpr static uint8_t cols = 3;
    uint8_t btn_width = (area.box.width / cols) - 2;

    uint16_t fg_color, bg_color, dimm_color;

    // Col 1
    Label lblMode{{0, padding_top, btn_width, btn_height}};
    Button btnGain{{0, btn_height + padding_top + 2, btn_width, btn_height}, &lcd, "", C565_BLACK};
    // Col 2
    Button btnVFO{{btn_width + 2, padding_top, btn_width, btn_height}, &lcd, "", C565_BLACK};
    Button btnSquelch{{btn_width + 2, btn_height + padding_top + 2, btn_width, btn_height}, &lcd, "", C565_BLACK};
    // Col 3
    Button btnRIT{{btn_width * 2 + 4, padding_top, btn_width, btn_height}, &lcd, "", C565_BLACK};
    Button btnSettings{{btn_width * 2 + 4, btn_height + padding_top + 2, btn_width, btn_height}, &lcd, "", C565_BLACK};

    char buf[20];

    void before_paint() override;

    void init();

    char *mode();

    char *squelch();

    char *gain();

    char *rit();

    char *vfo();

    void on_button(Button &button, st_inputEvent e);
};

#endif // TRX_RADIO_STATUS_WIDGET_H
