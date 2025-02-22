//
// Created by Angel Dust on 29/09/2024.
//

#include "radio_status_widget.h"
#include "../config.h"
#include "../main_board.h"
#include "../agc.h"
#include "Display_afb.h"
#include "menu_prompts.h"
#include "radio.h"
#include "printf.h"
#include "dsp/fft/fft.h"
#include "ui/button_widget.h"
#include "s_strength.h"
#include "menu.h"

void RadioStatusWidget::init() {

    btnSquelch.set_text("SQL");
    btnSquelch.set_two_lines(true);
    btnGain.set_text("Gain");
    btnGain.set_two_lines(true);
    btnVFO.set_text("VFO");
    btnVFO.set_two_lines(true);

    lblMode.on_select = [](Label &) {
        MODE mode;
        if (config.mode == ANALOG_RX || config.mode == ANALOG_TX) {
            mode = config.mode == ANALOG_RX ? ANALOG_TX : ANALOG_RX;
        } else {
            mode = config.mode == DIGITAL_RX ? DIGITAL_TX : DIGITAL_RX;
        }

        main_board::setMode(mode);
    };

    add_children({&lblMode, &btnSquelch, &btnGain, &btnVFO});

    for (Widget *btn : View::children()) {
        btn->set_font((FontDef *)&Font_Tiny8x8);
        btn->set_aling(ALIGN_CENTER);
        if (btn != &lblMode) {
            ((Button *)btn)->on_select = [this](Button &button) { this->on_button(button); };
        }
    }
}

void RadioStatusWidget::on_button(Button &button) {
    if (&button == &btnSquelch) {
        Menu::open_keypad<float>(
            sstrength::get_squelch(), "x1", "Squelch", 1, false, [](float v) { sstrength::set_squelch((float)v); }, 0, 9);
    } else if (&button == &btnGain) {
        Menu::open_gain();
    } else if (&button == &btnVFO) {
        radio::toggle_vfo();
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
    format_long(radio::get_vfo_frequency(radio::get_vfo() ? 0 : 1) / 1000, buf);
    return buf;
}

char *RadioStatusWidget::squelch() {
    if (!ISTX) {
        if (config.squelch_auto) {
            sprintf(buf, "auto");
        } else if (config.squelch_level == 0) {
            sprintf(buf, "0");
        } else {
            sprintf_(buf, "%0.1f", config.squelch_level); // force use custom sprintf (lib)
        }
    } else {
        sprintf(buf, "-");
    }

    return buf;
}

void RadioStatusWidget::before_paint() {

    st_radio_status status = {config.squelch_level, ISTX, radio::get_vfo(), agc::get_gain()};

    if (this->dirty() || !(status == _status)) { // Update only if status has changed

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
    }
}
