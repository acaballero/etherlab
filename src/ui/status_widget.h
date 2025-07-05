//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_STATUS_WIDGET_H
#define TRX_FRONTEND_STATUS_WIDGET_H

#include "ring_buffer.hpp"
#include "view.h"
#include "button_widget.h"
#include "../types.h"
#include "status.h"

#define BTN_WIDTH (DISPLAY_X_PIXELS / 6)
#define STATUS_MARGIN_TOP 2

class StatusWidget : public View {
  public:
    StatusWidget(Rect parent_rect) : View(parent_rect) {
        init();
    }

    StatusWidget(StatusWidget &&) = delete;

    bool on_input(const st_inputEvent e) override;

  protected:
    status::st_status _status;

    uint16_t fg_color, fg_color_auto, bg_color, dimm_color, disabled_color, disabled_bg;

    static constexpr uint8_t n_buttons = 6;
    enum DEFAULT_ACTIONS { MODULATION, FRONTEND, AGC, BAND, FILTER1, FILTER2 };

    Button buttons[n_buttons] = {{{0, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                 {{BTN_WIDTH, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                 {{BTN_WIDTH * 2, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                 {{BTN_WIDTH * 3, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                 {{BTN_WIDTH * 4, STATUS_MARGIN_TOP, BTN_WIDTH - 1, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK},
                                 {{BTN_WIDTH * 5, STATUS_MARGIN_TOP, BTN_WIDTH, area.box.height - STATUS_MARGIN_TOP}, display, "", C565_BLACK}};

    static Menu::menu_action_st default_actions_arr[n_buttons];

    static Menu::menu_actions_st default_actions;

    RingBuffer<Menu::menu_actions_st *, 4> actions_stack;

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

    void set_actions(Menu::menu_actions_st *);
    void set_action(uint8_t index, Menu::menu_action_st &action);
    bool push(Menu::menu_actions_st *);
    void pop();
};

#endif // TRX_FRONTEND_STATUS_WIDGET_H
