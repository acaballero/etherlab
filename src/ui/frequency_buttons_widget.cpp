//
// Created by Angel Dust on 03/01/2026.
//

#include "frequency_widget.h"
#include "Display_afb.h"
#include "config.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "main_board.h"
#include "radio.h"
#include "scanner.h"
#include "ui/button_widget.h"
#include "ui/frequency_buttons_widget.h"
#include "ui/frequency_memory_ui.h"
#include "ui/label_widget.h"
#include "ui/menu_frequency.h"
#include "view_manager.h"
#include "stdio.h"
#include "menu_prompts.h"

void FrequencyButtonsWidget::init() {

    set_focusable(true);

    scanner::signal.add(nullptr, [this](void *, const void *) {
        set_dirty();
    });

    btnVFO.action = [](Button &, st_inputEvent e) {
        if (e.ms > LONG_PRESS_MS) {
            freq_memory::toggle_memory_mode();
        } else {
            radio::toggle_vfo();
        }
    };
    btnScan.action = [](Button &, st_inputEvent) {
        scanner::toggle();
    };

    add_children({&btnRpt, &btnVFO, &btnScan});
    set_name("fbut");
    btnRpt.set_name("brpt");
    btnScan.set_name("bscn");
    btnVFO.set_name("bvfo");

    for (Widget *btn : View::children()) {
        btn->set_font((FontDef *)&Font_7x10);
        btn->set_aling(ALIGN_CENTER);
        ((Button *)btn)->set_style(ButtonStyle::BUTTON_STYLE_FLAT);
        ((Button *)btn)->set_bg(C565_VIOLET);
        ((Button *)btn)->set_fg(C565_WHITE);
    }

    btnRpt.set_color(C565_WHITE, C565_CYAN, C565_YELLOW);
    btnRpt.action = [](Button &, st_inputEvent) {
        Menu::open(Menu::repeaterMenu);
    };

    main_board::mode_signal.add(nullptr, [this](void *, const void *) {
        if (ISTX) {
            btnScan.set_visible(false);
            btnRpt.set_visible(false);
        } else {
            btnScan.set_visible(true);
            btnRpt.set_visible(true);
        }
    });
}

void FrequencyButtonsWidget::before_paint() {

    st_freqInfo freqInfo = {(unsigned long)radio::get_frequency(),
                            config.vfo[config.vfo_ix].step,
                            config.repeater_mode,
                            radio::get_vfo(),
                            freq_memory::memory_mode_on(),
                            config.mode};

    if (this->dirty() || !(freqInfo == this->status)) {

        if (scanner::scanner_config.status == scanner::SCANNER_STATUS_RUNNING) {
            if (scanner::scanner_config.direction == FORWARD) {
                btnScan.set_text("SCN >>");
            } else {
                btnScan.set_text("<< SCN");
            }

            btnScan.set_dimmed(false);
            btnRpt.set_enabled(false);
        } else {
            btnScan.set_text("SCN");
            btnScan.set_dimmed(true);
            btnRpt.set_enabled(true);
        }

        char buf[20];
        if (config.repeater_mode != RPT_MODE_OFF) {

            sprintf(buf, "%d", (int)config.repeater_offset / 1000);

            if (config.repeater_mode == RPT_MODE_NEGATIVE) {
                btnRpt.set_text(ISTX ? "TX+" : "RX-");
            } else {
                btnRpt.set_text(ISTX ? "RX-" : "TX+");
            }

            btnRpt.set_value(buf);
            btnRpt.set_dimmed(false);
        } else {
            btnRpt.set_dimmed(true);
            btnRpt.set_text("RPT");
            btnRpt.set_value("");
        }

        sprintf(buf, "%s%s", freq_memory::memory_mode_on() ? "M:" : "", radio::get_vfo() == 0 ? "A" : "B");

        btnVFO.set_text(buf);

        this->status = freqInfo;
        this->set_dirty();
    }
}
