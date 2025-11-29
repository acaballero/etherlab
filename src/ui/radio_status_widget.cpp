//
// Created by Angel Dust on 29/09/2024.
//

#include "radio_status_widget.h"
#include "../config.h"
#include "../main_board.h"
#include "../agc.h"
#include "Display_afb.h"
#include "input/inputEvent.h"
#include "menu_prompts.h"
#include "radio.h"
#include "printf.h"
#include "dsp/fft/fft.h"
#include "types.h"
#include "ui/button_widget.h"
#include "s_strength.h"
#include "menu.h"
#include "ui/frequency_memory_ui.h"
#include "ui/view_manager.h"

void RadioStatusWidget::init() {

    btnSquelch.set_text("SQL");
    btnSquelch.set_two_lines(true);
    btnGain.set_text("Gain");
    btnGain.set_two_lines(true);

    btnSettings.set_text("Menu");
    btnSettings.set_value("Mem");
    btnSettings.set_two_lines(true);

    lblMode.set_has_border(true);
    lblMode.set_name("mode");

    lblMode.on_select = [](Label &) {
        main_board::toggle_mode();
    };

    add_children({&btnSquelch, &btnGain, &btnVFO, &btnRIT, &btnSettings, &lblMode});

    for (Widget *btn : View::children()) {
        btn->set_font((FontDef *)&Font_Tiny8x8);
        btn->set_aling(ALIGN_CENTER);
        if (btn != &lblMode) {
            ((Button *)btn)->action = [this](Button &button, st_inputEvent e) {
                on_button(button, e);
            };
        }
    }
}

void RadioStatusWidget::on_button(Button &button, st_inputEvent e) {
    if (&button == &btnSquelch) {
        Menu::open_keypad<float>(
            sstrength::get_squelch(), "x1", "Squelch", 1, false,
            [](float v) {
                sstrength::set_squelch((float)v);
            },
            0, 9);
    } else if (&button == &btnGain) {
        Menu::open(Menu::frontendPathMenu);
    } else if (&button == &btnVFO) {
        if (freq_memory::memory_mode_on()) {
            freq_memory::set_next_prev(BACKWARDS, STATION);
        } else {
            radio::toggle_vfo();
        }
    } else if (&button == &btnRIT) {
        if (freq_memory::memory_mode_on()) {
            freq_memory::set_next_prev(FORWARD, STATION);
        } else {
            Menu::open_number_edit<int32_t>(
                radio::get_rit(), "Hz", "RIT", 0,
                [](int32_t v) {
                    radio::set_rit(v);
                },
                -500, 500, 1, 10);
        }
    } else if (&button == &btnSettings) {
        if (e.ms > LONG_PRESS_MS) {
            freq_memory::toggle_memory_mode();
        } else {
            Menu::open();
            if (freq_memory::memory_mode_on()) {
                nav.useMenu(freq_memory::freqMemMenu);
                nav.node().sel = freq_memory::get_current().id + 1;
            }
        }
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

char *RadioStatusWidget::rit() {
    sprintf(buf, "%d", radio::get_rit());
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

    st_radio_status status = {config.squelch_level, ISTX, radio::get_vfo(), agc::get_gain(), freq_memory::memory_mode_on()};

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
        display->set_padding(4, 4);
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

            btnSquelch.set_fg(fg_color);
            btnSquelch.set_bg(bg_color);
            btnGain.set_fg(fg_color);
            btnGain.set_bg(bg_color);

            lblMode.set_color(C565_GREEN);
            lblMode.set_bg(C565_TRANSPARENT);
            lblMode.set_style(ButtonStyle::LABEL_STYLE_HOLLOW);
        }

        if (freq_memory::memory_mode_on()) {
            btnVFO.set_text("\x80 Mem");
            btnVFO.set_two_lines(false);
            btnVFO.set_value("");
            btnRIT.set_text("Mem \x7F");
            btnRIT.set_two_lines(false);
            btnRIT.set_value("");
        } else {
            btnVFO.set_text("VFO");
            btnVFO.set_two_lines(true);
            btnRIT.set_text("RIT");
            btnRIT.set_two_lines(true);

            sprintf(buf, "VFO %s", radio::get_vfo() ? "A" : "B");
            btnVFO.set_text(buf);
            btnVFO.set_value(vfo());

            btnRIT.set_value(rit());
        }
    }
}
