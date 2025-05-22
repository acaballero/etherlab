//
// Created by Angel Dust on 17/04/2021.
//

#include "status_widget.h"
#include "../config.h"
#include "../main_board.h"
#include "input/inputEvent.h"
#include "printf.h"
#include "radio.h"
#include "ui/menu.h"
#include <cstddef>
#include <functional>

void StatusWidget::init() {

    StatusWidget *self = this;

    default_buttons[BAND].fn_writer = std::bind(&StatusWidget::band, self, &default_buttons[BAND]);
    default_buttons[FILTER1].fn_writer = std::bind(&StatusWidget::filter1, self, &default_buttons[FILTER1]);
    default_buttons[FILTER2].fn_writer = std::bind(&StatusWidget::filter2, self, &default_buttons[FILTER2]);

    for (Button &b : default_buttons) {
        add_child(&b);
    }

    for (auto &b : buttons) {
        add_child(&b);
        b.set_visible(false);
    }

    int i = 0;
    for (Widget *btn : View::children()) {
        ((Button *)btn)->set_font((FontDef *)&Font_Tiny8x8);

        char name[6];
        sprintf(name, "stb-%d", i++);
        btn->set_name(name);
    }

    // Subscribe to published actions
    actions_signal.add(this, [this](void *, void *params) {
        if (params == nullptr) {
            set_defaults();
        } else {
            Menu::menu_actions_st actions = *((Menu::menu_actions_st *)params);
            for (size_t i = 0; i < n_buttons; i++) {

                if (i < actions.size) {
                    set_action(i, actions.actions[i]);
                    buttons[i].set_visible(true);

                } else {
                    buttons[i].set_visible(false);
                }

                default_buttons[i].set_visible(false);

                set_dirty();
            }
        }
    });
}

void StatusWidget::set_defaults() {

    for (Button &b : default_buttons) {
        b.set_visible(true);
    }

    for (Button &b : buttons) {
        b.set_visible(false);
    }
}

void StatusWidget::set_action(uint8_t index, Menu::menu_action_st &menu_action) {
    Button *button = &buttons[index];
    button->action = [&menu_action](Button &, st_inputEvent) {
        menu_action.action();
    };
    button->set_text(menu_action.name);
    button->set_visible(true);
}

void StatusWidget::mode() {
    sprintf(buf, ISTX ? "TX" : "RX");
}

const char *StatusWidget::modulation() {
    return radio::modulation_names[config.modulation];
}

char *StatusWidget::frontend() {
    if (!ISTX) {

        switch (config.frontend_path) {
            case radio::FRONTEND_PATH_THRU:
                sprintf(buf, "ATT:0");
                break;
            case radio::FRONTEND_PATH_ATT:
                sprintf(buf, "ATT:10");
                break;
            case radio::FRONTEND_PATH_LNA:
                sprintf(buf, "LNA");
                break;
        }
    } else {
        sprintf(buf, "-");
    }
    return buf;
}

char *StatusWidget::agc_alc() {
    if (!ISTX) {
        sprintf(buf, "AGC");
        default_buttons[AGC].set_fg(fg_color);
    } else {
        sprintf(buf, "ALC");
    }

    return buf;
}

void StatusWidget::filter1(Widget *) {
    if (config.filter < radio::BAND_AUTO) {
        sprintf(buf, "%.4s", radio::bandNames[config.filter]);
    } else {
        if (this->_status.filter == radio::BAND_ALL) {
            sprintf(buf, "*");
        } else {
            sprintf(buf, "%.4s", this->_status.filter >= radio::BAND_AUTO ? "None" : radio::bandNames[this->_status.filter]);
        }
    }

    display->print("}:", buf, "", dimm_color, config.filter < radio::BAND_AUTO ? fg_color : fg_color_auto, dimm_color);
}

void StatusWidget::filter2(Widget *) {

    sprintf(buf, "%s", radio::IFFilterNames[radio::if_filter]);

    display->print("~:", buf, "", dimm_color, config.if_filter != radio::IF_FILTER_AUTO ? fg_color : fg_color_auto, dimm_color);
}

void StatusWidget::band(Widget *) {

    display->print("B:");

    if (config.band == radio::BAND_NONE) {
        display->print("-");
    } else {

        if (config.band == radio::BAND_ALL) {
            display->print("* ");
        }

        display->setColor(config.band == radio::BAND_AUTO ? fg_color_auto : fg_color);

        display->print(radio::bandNames[radio::get_band()]);
    }
}

void StatusWidget::before_paint() {

    radio::BAND band = config.band == radio::BAND_AUTO ? radio::find_band(radio::get_frequency()) : config.band;

    status::st_status status = {config.modulation, ISTX, band, radio::filter, radio::if_filter, config.frontend_path, config.agc_enabled,
                                _status.f_carrier, // we won't show the frequency in the status bar, so use current_status value
                                Menu::menuStatus

    };

    if (this->dirty() || !(status == _status)) { // Update only if status has changed

        _status = status;
        this->set_dirty();
        if (ISTX) {
            fg_color = C565_BLACK;
            bg_color = C565_WHITE;
            dimm_color = C565_BLACK;
            disabled_bg = C565_GREY_DARK;
            fg_color_auto = C565_MAGENTA;

            default_buttons[AGC].set_enabled(false);

        } else {
            fg_color = C565_BLACK;
            bg_color = C565_WHITE;
            dimm_color = C565_BLACK;
            disabled_color = C565_GREY_DARK;
            disabled_bg = C565_GREY_LIGHT;
            fg_color_auto = C565_MAGENTA;

            default_buttons[AGC].set_dimmed(!config.agc_enabled);
        }

        default_buttons[BAND].set_enabled(!ISTX);
        default_buttons[FILTER1].set_enabled(!ISTX);
        default_buttons[FILTER2].set_enabled(!ISTX);
        default_buttons[FRONTEND].set_enabled(!ISTX);

        display->setBgColor(bg_color);
        display->setColor(fg_color);

        display->setFont((FontDef *)&Font_Tiny8x8);
        display->setVerticalLineSpacing(6);
        display->setPadding(4, 4);
        display->gotoCharXY(0, 0);

        const char *modulation_str = modulation();
        default_buttons[MODULATION].set_text(modulation_str);
        default_buttons[FRONTEND].set_text(frontend());
        default_buttons[AGC].set_text(agc_alc());

        for (Widget *btn : View::children()) {
            ((Button *)btn)->set_bg(bg_color);
            btn->set_aling(ALIGN_CENTER);
            btn->set_dirty();
        }
    }
}

bool StatusWidget::on_input(const st_inputEvent e) {
    bool consumed = true;
    // bool long_press, very_long_press;

    switch (e.type) {

        case INPUT_EVENT_TYPE_BUTTON_PRESS:
        case INPUT_EVENT_TYPE_BUTTON_DBL_PRESS:

            switch (e.value) {

                case FPANEL_DISPLAY_BUTTON_1: //  MODULATION
                    if (buttons[0].visible()) {
                        buttons[0].action(buttons[0], e);
                    } else {
                        Menu::open(Menu::modulationMenu);
                    }
                    break;
                case FPANEL_DISPLAY_BUTTON_2: //  FRONTEND
                    if (buttons[1].visible()) {
                        buttons[1].action(buttons[0], e);
                    } else {
                        Menu::open(Menu::frontendPathMenu);
                    }
                    break;
                case FPANEL_DISPLAY_BUTTON_3: // AGC
                    if (buttons[2].visible()) {
                        buttons[2].action(buttons[0], e);
                    } else {
                        config.agc_enabled = !config.agc_enabled;
                    }
                    main_board::update();
                    break;
                case FPANEL_DISPLAY_BUTTON_5: // FILTER 1
                    if (buttons[3].visible()) {
                        buttons[3].action(buttons[0], e);
                    } else {
                        Menu::open(Menu::filterMenu);
                    }
                    break;
                case FPANEL_DISPLAY_BUTTON_6: // FILTER 2
                    if (buttons[4].visible()) {
                        buttons[4].action(buttons[0], e);
                    } else {
                        Menu::open(Menu::IFFilterMenu);
                    }
                    break;
                case FPANEL_DISPLAY_BUTTON_4: // BAND
                    if (buttons[0].visible()) {
                        buttons[0].action(buttons[0], e);
                    } else {
                        Menu::open(Menu::bandMenu);
                    }
                    break;
                default:
                    consumed = false;
            }
            break;
        case INPUT_EVENT_TYPE_BUTTON_RELEASE:

            switch (e.value) {
                case FPANEL_DISPLAY_BUTTON_1:
                case FPANEL_DISPLAY_BUTTON_2:
                case FPANEL_DISPLAY_BUTTON_3:
                case FPANEL_DISPLAY_BUTTON_4:
                case FPANEL_DISPLAY_BUTTON_5:
                case FPANEL_DISPLAY_BUTTON_6:
                    break;
                default:
                    consumed = false;
            }

            break;
        default:
            consumed = false;
            break;
    }

    return consumed;
}
