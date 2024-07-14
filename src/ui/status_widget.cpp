//
// Created by Angel Dust on 17/04/2021.
//

#include "status_widget.h"
#include "../config.h"
#include "../main_board.h"
#include "radio.h"

void StatusWidget::paint_callback() {

    char buf[20];
    display->clear();

    uint16_t fg_color, fg_color_auto, bg_color, dimm_color, disabled_color, disabled_bg;

    if (ISTX) {
        fg_color = C565_GREY_LIGHT;
        bg_color = C565_RED;
        dimm_color = C565_BLACK;
        disabled_bg = C565_GREY_DARK;
        fg_color_auto = C565_WHITE;
    } else {
        fg_color = C565_BLACK;
        bg_color = C565_WHITE;
        dimm_color = C565_OLIVE;
        disabled_color = C565_GREY_DARK;
        disabled_bg = C565_GREY_LIGHT;
        fg_color_auto = C565_MAGENTA;
    }

    display->fillBuffer(bg_color);
    display->setBgColor(bg_color);
    display->setColor(fg_color);

    display->setFont((FontDef *) &Font_Tiny8x8);
    display->setVerticalLineSpacing(6);
    display->setPadding(4,4);
    display->gotoCharXY(0, 0);
    display->print(radio::modulationNames[config.modulation]);
    display->print(ISTX ? " TX" : " RX");

    if (!ISTX) {
        print_separator();
        switch (config.frontend_path) {
            case radio::FRONTEND_PATH_THRU:
                display->print("ATT:0");
                break;
            case radio::FRONTEND_PATH_ATT:
                display->print("ATT:10");
                break;
            case radio::FRONTEND_PATH_LNA:
                display->print("LNA");
                break;
        }

        print_separator();

        if (!config.agc_enabled) {
            display->setColor(disabled_color);
            //display->setBgColor(disabled_bg);
        }

        display->print("AGC");
        display->setColor(fg_color);
        display->setBgColor(bg_color);

    } else {
//        if (config.alc_enabled) {
//            print_separator();
//            display->print("ALC");
//        }
    }

    if (config.filter < radio::BAND_AUTO) {
        sprintf(buf, "%.3s", radio::bandNames[config.filter]);
    } else {
        if (this->_status.filter == radio::BAND_ALL) {
            sprintf(buf, "*");
        } else {
            sprintf(buf, "%.3s", radio::bandNames[this->_status.filter]);
        }
    }
    print_separator();
    display->print("}:", buf, "", dimm_color, config.filter < radio::BAND_AUTO ? fg_color : fg_color_auto, dimm_color);

    if (config.if_filter != radio::IF_FILTER_AUTO) {
        sprintf(buf, "%s", radio::IFFilterNames[config.if_filter]);
    } else {
        sprintf(buf, "%s", radio::IFFilterNames[this->_status.if_filter]);
    }
    print_separator();
    display->print("~:", buf, "", dimm_color, config.filter < radio::BAND_AUTO ? fg_color : fg_color_auto, dimm_color);

    //display->gotoCharXY(15, 13);
    print_separator();
    display->print("B:");

    if (config.band < radio::BAND_AUTO) {
        display->print(radio::bandNames[config.band]);
    } else {
        if (config.band == radio::BAND_NONE) {
            display->print("*");
        }

        if (this->_status.band < radio::BAND_AUTO) {
            display->setColor(config.filter < radio::BAND_AUTO ? fg_color : fg_color_auto);
            display->print(radio::bandNames[this->_status.band]);
        }
    }

    if (!ISTX) {
        if (config.squelch_auto) {
            sprintf(buf, "A");
        } else {
            sprintf(buf, "%.1f", config.squelch_level);
        }
        print_separator();
        display->print("S:", buf, "", dimm_color, fg_color, dimm_color);
    }

    display->setVerticalLineSpacing(2);
    display->setFont((FontDef *) &Font_11x18);

    display->gotoXY(DISPLAY_X_PIXELS - 12, 0);
    if (main_board::getMute()) display->setColor(C565_GREY_DARK);
    display->write(main_board::getMute() ? '}' : '|'); // characters for mute on/off icons

}

void StatusWidget::print_separator() {
    const FontDef *font = display->getFont();
    uint8_t vls = display->getVerticalLineSpacing();
    uint16_t py = display->get_padding_y();
    display->setVerticalLineSpacing(0);
    display->setPadding(4, 0);
    display->setFont((FontDef *) &Font_11x18);
    display->print("{");
    display->setFont(font);
    display->setPadding(4, py);
    display->setVerticalLineSpacing(vls);
}

void StatusWidget::do_paint() {

    radio::BAND band = config.band == radio::BAND_AUTO ? radio::find_band(radio::get_frequency()) : config.band;

    st_status status = {
            config.squelch_level,
            config.modulation,
            ISTX,
            band,
            radio::filter,
            radio::if_filter,
            config.frontend_path,
            config.agc_enabled,
            _status.f_carrier, // we won't show the frequency in the status bar, so use current_status value
            main_board::getMute() ? true : false
    };

    if (this->dirty() || !(status == _status)) { // Update only if status has changed
        this->set_dirty();
        _status = status;
        display->drawArea(&this->area, this);
    }
}