//
// Created by Angel Dust on 29/09/2024.
//

#include "radio_status_widget.h"
#include "../config.h"
#include "../main_board.h"
#include "../agc.h"
#include "Display_afb.h"
#include "radio.h"
#include "ui/button_widget.h"

void RadioStatusWidget::init() {

    btnSquelch.set_text("SQL");
    btnSquelch.set_two_lines(true);
    btnGain.set_text("Gain");
    btnGain.set_two_lines(true);
    btnVFO.set_text("VFO");
    btnVFO.set_two_lines(true);

    add_children({&lblMode, &btnSquelch, &btnGain, &btnVFO});

    for (Widget *btn : View::children()) {
        btn->set_font((FontDef *)&Font_Tiny8x8);
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

char *RadioStatusWidget::vfo() {
    format_long(radio::get_vfo_frequency(radio::get_vfo()), buf);
    return buf;
}

char *RadioStatusWidget::squelch() {
    if (!ISTX) {
        if (config.squelch_auto) {
            sprintf(buf, "auto");
        } else if (config.squelch_level == 0) {
            sprintf(buf, "0");
        } else {
            sprintf(buf, " %0.1f", config.squelch_level);
        }
    }

    return buf;
}

void RadioStatusWidget::do_paint() {

    st_radio_status status = {config.squelch_level, ISTX, radio::get_vfo(), agc::get_gain()};

    if (this->dirty() || !(status == _status)) { // Update only if status has changed

        this->set_dirty();
        display->drawArea(&this->area, this);

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

        display->setFont((FontDef *)&Font_Tiny8x8);
        display->setVerticalLineSpacing(6);
        display->setPadding(4, 4);
        display->gotoCharXY(0, 0);

        lblMode.set_label(mode());

        if (ISTX) {

            btnSquelch.set_visible(false);
            btnGain.set_visible(false);

            lblMode.set_color(C565_WHITE);
            lblMode.set_bg(C565_RED);
            lblMode.set_style(ButtonStyle::BUTTON_STYLE_FLAT);

        } else {

            btnSquelch.set_visible(true);
            btnGain.set_visible(true);

            btnSquelch.set_value(squelch());
            btnGain.set_value(gain());

            sprintf(buf, "VFO %s", radio::get_vfo() ? "A" : "B");
            btnVFO.set_text(buf);
            btnVFO.set_value(vfo());

            btnSquelch.set_fg(fg_color);
            btnSquelch.set_bg(bg_color);
            btnGain.set_fg(fg_color);
            btnGain.set_bg(bg_color);

            lblMode.set_color(C565_GREEN);
            lblMode.set_bg(C565_TRANSPARENT);
            lblMode.set_style(ButtonStyle::LABEL_STYLE_HOLLOW);
        }

        // Selectively paint all children.
        for (const auto child : this->children()) {
            if (child->visible()) {
                child->set_dirty();
                child->paint();
                child->set_clean();
            }
        }
    }
}

void RadioStatusWidget::paint_callback() {
    // TODO: Clear only the first time. Now causes flicker (first black then the buttons)
    display->clear();
}
