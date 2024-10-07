//
// Created by Angel Dust on 29/09/2024.
//

#include "radio_status_widget.h"
#include "../config.h"
#include "../main_board.h"
#include "../agc.h"

void RadioStatusWidget::init() {

    btnSquelch.set_text("SQL");
    btnGain.set_text("Gain");

    add_children({&lblMode, &btnSquelch, &btnGain});

    for (Widget *btn: View::children()) {
        btn->set_font((FontDef *) &Font_Tiny8x8);
        btn->set_aling(ALIGN_CENTER);
    }
}

char *RadioStatusWidget::gain() {
    snprintf(buf, 4, "%d", agc::get_gain());
    return buf;
}

char *RadioStatusWidget::mode() {
    sprintf(buf, ISTX ? "TX" : "RX");
    return buf;
}

char *RadioStatusWidget::squelch() {
    if (!ISTX) {
        if (config.squelch_auto) {
            sprintf(buf, "A");
        } else {
            sprintf(buf, " %.1f", config.squelch_level);
        }
    }

    return buf;
}

void RadioStatusWidget::do_paint() {

    radio::BAND band = config.band == radio::BAND_AUTO ? radio::find_band(radio::get_frequency()) : config.band;

    st_radio_status status = {
            config.squelch_level,
            ISTX
    };

    if (this->dirty() || !(status == _status)) { // Update only if status has changed

        display->drawArea(&this->area, this);
        this->set_dirty();
        _status = status;

        if (ISTX) {
            fg_color = C565_GREY_LIGHT;
            bg_color = C565_WHITE;
            dimm_color = C565_BLACK;

        } else {
            fg_color = C565_BLACK;
            bg_color = C565_WHITE;
            dimm_color = C565_BLACK;
        }

        display->setBgColor(bg_color);
        display->setColor(fg_color);

        display->setFont((FontDef *) &Font_Tiny8x8);
        display->setVerticalLineSpacing(6);
        display->setPadding(4, 4);
        display->gotoCharXY(0, 0);

        // Labels
        lblMode.set_label(mode());
        lblMode.set_color(ISTX ? C565_RED : C565_GREEN);

        // Buttons
        if (ISTX) {
            btnSquelch.set_visible(false);
            btnGain.set_visible(false);
        }
        else {
            btnSquelch.set_visible(true);
            btnGain.set_visible(true);

            btnSquelch.set_value(squelch());
            btnGain.set_value(gain());

            btnSquelch.set_fg(fg_color);
            btnSquelch.set_bg(bg_color);
            btnGain.set_fg(fg_color);
            btnGain.set_bg(bg_color);
        }

        // Selectively paint all children.
        for (const auto child: this->children()) {
            if (child->visible()) {
                child->set_dirty();
                child->paint();
                child->set_clean();
            }
        }
    }
}

void RadioStatusWidget::paint_callback() {
    display->clear();
}

