//
// Created by Angel Dust on 18/04/2021.
//

#include "frequency_widget.h"
#include "Display_afb.h"
#include "config.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "radio.h"
#include "scanner.h"
#include "ui/button_widget.h"
#include "ui/frequency_memory_ui.h"
#include "ui/label_widget.h"
#include "ui/menu_frequency.h"
#include "view_manager.h"
#include "stdio.h"
#include "menu_prompts.h"

void FrequencyWidget::init() {

    scanner::signal.add(nullptr, [this](void *, void *) { set_dirty(); });

    btnVFO.on_select = [](Button &) { radio::toggle_vfo(); };
    btnScan.on_select = [](Button &) { scanner::toggle(); };

    add_children({&btnRpt, &btnVFO, &btnScan, &freqWidget});
    set_name("freq_w");

    for (Widget *btn : View::children()) {
        btn->set_font((FontDef *)&Font_7x10);
        btn->set_aling(ALIGN_CENTER);
        ((Button *)btn)->set_style(ButtonStyle::BUTTON_STYLE_FLAT);
        ((Button *)btn)->set_bg(C565_VIOLET);
        ((Button *)btn)->set_fg(C565_WHITE);
    }

    btnRpt.set_color(C565_WHITE, C565_CYAN, C565_YELLOW);
    btnRpt.on_select = [](Button &) { Menu::open(Menu::repeaterMenu); };
}

void FrequencyWidget::before_paint() {

    st_freqInfo freqInfo = {(unsigned long)radio::get_frequency(), config.vfo[config.vfo_ix].step, config.repeater_mode, radio::get_vfo()};

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
        if (config.repeater_mode != radio::RPT_MODE_OFF) {

            sprintf(buf, "%d", (int)config.repeater_offset / 1000);

            if (config.repeater_mode == radio::RPT_MODE_NEGATIVE) {
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

        btnVFO.set_text(radio::get_vfo() == 0 ? "A" : "B");
        this->status = freqInfo;
        this->set_dirty();
    }
}

bool FrequencyWidget::on_touch(const st_inputEvent e) {

    if (e.ms > LONG_PRESS_MS) {
        freq_memory::open_save_current();

    } else {
        Menu::open_keypad<uint64_t>(
            radio::get_frequency(), "Hz", "Frequency", 0, true, [](uint64_t v) { radio::set_frequency((uint64_t)v); }, 0, 0);
    }
    return true;
}

void FrequencyWidgetInner::paint_callback() {

    char buf[20];

    uint16_t fg_color;

    display->clear();

    format_long(radio::get_frequency(), buf);

    display->setBgColor(C565_TRANSPARENT);

    if (scanner::scanner_config.status == scanner::SCANNER_STATUS_RUNNING) {
        fg_color = C565_MAGENTA;
    } else {
        fg_color = C565_YELLOW;
    }

    FontDef *font = (FontDef *)&Font_Digits13x16;

    uint8_t dec_place = (uint8_t)log10((double)config.vfo[config.vfo_ix].step) + 1;
    uint8_t trim = font->trim_punct_end + font->trim_punct_start;
    uint16_t start_line = area.box.width - (dec_place * font->width) - trim / 2;

    uint16_t x = area.box.width - strlen(buf) * font->width + trim;

    display->writeString(x, 0, buf, font, fg_color, C565_TRANSPARENT);
    display->setColor(C565_GREY_LIGHT);

    if (dec_place > 3) {
        start_line -= font->width - trim; // sip hundreds separator
    }
    if (dec_place > 6) {
        start_line -= font->width - trim; // skip thousands separator
    }

    display->writeRect(start_line + 2, 16, start_line + 2, 17);
    display->writeRect(start_line + 1, 17, start_line + 3, 17);
    display->writeRect(start_line, 18, start_line + 4, 18);
}

void FrequencyWidgetInner::before_paint() {}
