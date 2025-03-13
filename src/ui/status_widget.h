//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_STATUS_WIDGET_H
#define TRX_FRONTEND_STATUS_WIDGET_H

#include "view.h"
#include "button_widget.h"
#include "../types.h"
#include "status.h"

#define BTN_WIDTH (DISPLAY_X_PIXELS / 6)
#define STATUS_MARGIN_TOP 2

class StatusWidget : public View {
  public:
    StatusWidget(Rect parent_rect) : View(parent_rect) { init(); }

    bool on_input(const st_inputEvent e) override;

  protected:
    status::st_status _status;

    uint16_t fg_color, fg_color_auto, bg_color, dimm_color, disabled_color, disabled_bg;

    static constexpr uint8_t n_buttons = 6;
    enum DEFAULT_ACTIONS { MODULATION, FRONTEND, AGC, BAND, FILTER1, FILTER2 };

    Button default_buttons[n_buttons] = {{{0, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                         {{BTN_WIDTH, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                         {{BTN_WIDTH * 2, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                         {{BTN_WIDTH * 3, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                         {{BTN_WIDTH * 4, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                         {{BTN_WIDTH * 5, STATUS_MARGIN_TOP, BTN_WIDTH, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK}};

    Button buttons[n_buttons] = {{{0, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                 {{BTN_WIDTH, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                 {{BTN_WIDTH * 2, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                 {{BTN_WIDTH * 3, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                 {{BTN_WIDTH * 4, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                 {{BTN_WIDTH * 5, STATUS_MARGIN_TOP, BTN_WIDTH, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK}};

    char buf[20];

    void init();

    void mode();

    const char *modulation();

    void band(Widget *);

    void filter1(Widget *);

    void filter2(Widget *);

    char *frontend();

    char *agc_alc();

    void before_paint() override;

    void set_action(uint8_t index, Menu::menu_action_st &action);
    void set_defaults();
};

#endif // TRX_FRONTEND_STATUS_WIDGET_H
