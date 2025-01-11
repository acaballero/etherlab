//
// Created by Angel Dust on 17/04/2021.
//

#include <menuIO/stringIn.h>
#include "input/inputEvent.h"
#include "menu_widget.h"
#include "menu.h"
#include "config.h"
#include "../input/input.h"
#include "radio.h"

using namespace Menu;

stringIn<1> strIn;

void MenuWidget::paint_callback() {

    // display->fillBuffer(0);
    display->clear();

    // display->gotoXY(0, 3);
    display->setFont((FontDef *)&Font_7x10);
    // display->write("HELLO");
    nav.doOutput();
}

void MenuWidget::do_paint() {

    if (this->dirty()) {
        display->drawArea(&this->area, this);
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

            switch (e.value) {

                case FPANEL_PAD_BUTTON_2: // VFO
                    radio::toggle_vfo();
                    break;
                case FPANEL_PAD_BUTTON_3: // GAIN
                    nav.doNav(navCmd(enterCmd));
                    nav.doNav(navCmd(idxCmd, 0));
                    nav.doNav(navCmd(idxCmd, 4));
                    break;
                case FPANEL_PAD_BUTTON_4: // SQuelch
                    nav.doNav(navCmd(enterCmd));
                    nav.doNav(navCmd(idxCmd, 0));
                    nav.doNav(navCmd(idxCmd, 5));
                    break;
                // case KEY_BACK:
                case FPANEL_PAD_BUTTON_5:
                    strIn.write('/');
                    nav.doInput(strIn);
                    break;
                case BTN_ENCODER:

                    long_press = e.ms > 500;
                    very_long_press = e.ms > 3000;

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

                case FPANEL_DISPLAY_BUTTON_1: // LEFT / MODULATION

                    if (menuStatus == ACTIVE) {
                        strIn.write('-');
                        nav.doInput(strIn);

                    } else {
                        nav.doNav(navCmd(enterCmd));
                        nav.doNav(navCmd(idxCmd, 0));
                        nav.doNav(navCmd(idxCmd, 0));
                    }
                    break;
                case FPANEL_DISPLAY_BUTTON_2: // RIGHT / FRONTEND
                    if (menuStatus == ACTIVE) {
                        strIn.write('+');
                        nav.doInput(strIn);
                    } else {
                        nav.doNav(navCmd(enterCmd));
                        nav.doNav(navCmd(idxCmd, 4));
                        nav.doNav(navCmd(idxCmd, 3));
                    }
                    break;
                case FPANEL_DISPLAY_BUTTON_3: // AGC
                    nav.doNav(navCmd(enterCmd));
                    nav.doNav(navCmd(idxCmd, 0));
                    nav.doNav(navCmd(idxCmd, 4));
                    break;
                case FPANEL_DISPLAY_BUTTON_4: // FILTER 1
                    nav.doNav(navCmd(enterCmd));
                    nav.doNav(navCmd(idxCmd, 0));
                    nav.doNav(navCmd(idxCmd, 2));
                    break;
                case FPANEL_DISPLAY_BUTTON_5: // FILTER 2
                    nav.doNav(navCmd(enterCmd));
                    nav.doNav(navCmd(idxCmd, 0));
                    nav.doNav(navCmd(idxCmd, 3));
                    break;
                case FPANEL_DISPLAY_BUTTON_6: // BAND

                    nav.doNav(navCmd(enterCmd));
                    nav.doNav(navCmd(idxCmd, 0));
                    nav.doNav(navCmd(idxCmd, 1));
                    break;

                default:

                    consumed = false;
                    break;
            }

            break;

        case INPUT_EVENT_TYPE_ENCODER:

            if (menuStatus == ACTIVE) {
                strIn.write(e.value > 0 ? '+' : '-');
                if (!e.value)
                    strIn.write(' ');
                nav.doInput(strIn);
            } else {
                consumed = false;
            }
            break;
        default:
            consumed = false;
    }

    return consumed;
}
