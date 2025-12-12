//
// Created by Angel Dust on 17/04/2021.
//

#include <menuIO/stringIn.h>
#include "input/inputEvent.h"
#include "menuBase.h"
#include "menu_widget.h"
#include "menu.h"
#include "config.h"
#include "../input/input.h"
#include "radio.h"
#include "main_board.h"

using namespace Menu;

stringIn<1> strIn;

bool MenuWidget::paint_callback() {

    display->clear();

    display->setFont((FontDef *)&Font_7x10);

    nav.doOutput();

    return true;
}

void MenuWidget::before_paint() {
    if (nav.active().dirty) {
        this->set_dirty();
    }
}

bool MenuWidget::on_input(const st_inputEvent e) {

    bool consumed = true;
    bool long_press, very_long_press;

    switch (e.type) {

        case INPUT_EVENT_TYPE_BUTTON_PRESS:
        case INPUT_EVENT_TYPE_BUTTON_DBL_PRESS:

            // if (e.value != BTN_ENCODER && e.value != KEY_BACK) {
            //     if (menuStatus == ACTIVE) {
            //         // If a key from the keypad is pressed we close the menu to start navigating from scratch
            //         menu_exit();
            //         return true;
            //     }
            // }
            //

            switch (e.value) {

                case FPANEL_PAD_BUTTON_2: // VFO
                    radio::toggle_vfo();
                    break;
                case FPANEL_PAD_BUTTON_3: // GAIN
                    Menu::open(Menu::frontendPathMenu);
                    break;
                case FPANEL_PAD_BUTTON_4: // SQuelch
                    Menu::open(Menu::squelchEditMenu);
                    break;
                    // case KEY_BACK:
                case FPANEL_PAD_BUTTON_5:
                    if (Menu::menuStatus == ACTIVE) {
                        nav.doNav(escCmd);
                    } else {
                        consumed = false;
                    }
                    break;
                case FPANEL_PAD_BUTTON_6:

                    long_press = e.ms > LONG_PRESS_MS;
                    very_long_press = e.ms > VERY_LONG_PRESS_MS;

                    if (!very_long_press) {
                        if (long_press) {
                            strIn.write('?'); // idxCmd
                            nav.doInput(strIn);
                        } else {
                            strIn.write('*');
                            nav.doInput(strIn);
                        }
                    } else {
                        consumed = false;
                    }

                    break;
                default:

                    consumed = false;
                    break;
            }

            break;

        case INPUT_EVENT_TYPE_ENCODER:

            if (Menu::menuStatus == ACTIVE) {
                strIn.write(e.value > 0 ? '+' : '-');
                if (!e.value) {
                    strIn.write(' ');
                }
                nav.doInput(strIn);
            } else {
                consumed = false;
            }
            break;
        default:
            consumed = false;
    }

    if (consumed) {
        set_dirty();
    }

    return consumed;
}
