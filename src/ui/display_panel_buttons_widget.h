//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_DISPLAY_PANEL_BUTTONS_WIDGET_H
#define TRX_FRONTEND_DISPLAY_PANEL_BUTTONS_WIDGET_H

#include "view.h"
#include "button_widget.h"
#include "../types.h"
#include <sys/_stdint.h>

class DisplayPanelButtonsWidget : public View {

  protected:
    static constexpr uint8_t n_buttons = 6;

    uint16_t fg_color, fg_color_auto, bg_color, dimm_color, disabled_color, disabled_bg;

    Button buttons[n_buttons];

    void init();

    void before_paint() override;

  public:
    DisplayPanelButtonsWidget(Rect parent_rect) : View(parent_rect) { init(); }

    void paint_callback() override;

    void set_labels(const char **labels);
    Button *get_buttons();
};

#endif // TRX_FRONTEND_DISPLAY_PANEL_BUTTONS_WIDGET_H
