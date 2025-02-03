//
// Created by Angel Dust on 18/04/2021.
//

#include "frequency_widget.h"
#include "Display_afb.h"
#include "config.h"
#include "radio.h"
#include "scanner.h"
#include "ui/button_widget.h"
#include "view_manager.h"
#include <stdint.h>

void FrequencyWidget::init() {

    add_children({&lblRpt, &lblVFO, &freqWidget});

    for (Widget *lbl : View::children()) {
        lbl->set_font((FontDef *)&Font_7x10);
        lbl->set_aling(ALIGN_CENTER);
        ((Label *)lbl)->set_style(ButtonStyle::BUTTON_STYLE_FLAT);
        ((Label *)lbl)->set_bg(C565_VIOLET);
        ((Label *)lbl)->set_color(C565_WHITE, C565_CYAN, C565_WHITE);
    }
}

void FrequencyWidget::before_paint() {

    st_freqInfo freqInfo = {(unsigned long)radio::get_frequency(), config.vfo[config.vfo_ix].step, config.repeater_mode, radio::get_vfo()};

    if (this->dirty() || !(freqInfo == this->status)) {

        char buf[20];
        if (config.repeater_mode != radio::RPT_MODE_OFF) {

            sprintf(buf, "%d", (int)config.repeater_offset / 1000);

            if (config.repeater_mode == radio::RPT_MODE_NEGATIVE) {
                lblRpt.set_label(ISTX ? "TX+" : "RX-");
            } else {
                lblRpt.set_label(ISTX ? "RX-" : "TX+");
            }

            lblRpt.set_value(buf);
            lblRpt.set_visible(true);
        } else {
            lblRpt.set_visible(false);
        }

        lblVFO.set_label(radio::get_vfo() == 0 ? "A" : "B");
        this->status = freqInfo;
        this->set_dirty();
    }
}

bool FrequencyWidget::on_input(const st_inputEvent event) {
    switch (event.type) {

        case INPUT_EVENT_TYPE_TOUCH_END:

            Menu::open_keypad<uint64_t>(
                radio::get_frequency(), "Hz", "Frequency", 0, false, [](uint64_t v) { radio::set_frequency((uint64_t)v); }, 0, 0);

            return true;
        default:
            return false;
    }

    return false;
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

    FontDef *font = (FontDef *)&Font_11x18;

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
