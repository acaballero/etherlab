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
#include <sys/_stdint.h>

void FrequencyWidget::init() {

    add_children({&lblRpt, &lblVFO});

    for (Widget *lbl : View::children()) {
        lbl->set_font((FontDef *)&Font_7x10);
        lbl->set_aling(ALIGN_CENTER);
        ((Label *)lbl)->set_style(ButtonStyle::BUTTON_STYLE_FLAT);
        ((Label *)lbl)->set_bg(C565_VIOLET);
        ((Label *)lbl)->set_color(C565_WHITE, C565_CYAN, C565_WHITE);
    }
}

void FrequencyWidget::paint_callback() {

    char buf[20], buf2[20];
    uint16_t fg_color;

    display->clear();

    if (config.repeater_mode != radio::RPT_MODE_OFF) {

        sprintf(buf, "%d", config.repeater_offset / 1000);

        if (config.repeater_mode == radio::RPT_MODE_NEGATIVE) {
            lblRpt.set_label("RX-");
        } else {
            lblRpt.set_label("TX+");
        }

        lblRpt.set_value(buf);
        lblRpt.set_visible(true);
    } else {
        lblRpt.set_visible(false);
    }

    lblVFO.set_label(radio::get_vfo() == 0 ? "A" : "B");

    format_long(radio::get_frequency(), buf2);
    sprintf(buf, "%12s", buf2);

    display->setBgColor(C565_TRANSPARENT);

    if (scanner::scanner_config.status == scanner::SCANNER_STATUS_RUNNING) {
        fg_color = C565_MAGENTA;
    } else {
        fg_color = C565_YELLOW;
    }

    uint16_t x = (area.width / 4) + 24 + 4;

    display->writeString(x, 0, buf, (FontDef *)&Font_11x18, fg_color, C565_TRANSPARENT);
    display->setColor(C565_GREY_LIGHT);
    display->setFont((FontDef *)&Font_7x10);
    display->setPadding(0, 5);
    display->print("Hz");

    uint8_t dec_place = 10 - (uint8_t)log10((double)config.vfo[config.vfo_ix].step);
    uint16_t start_line = x + (dec_place * 11);

    if (config.vfo[config.vfo_ix].step > 100) {
        start_line -= 6; // sip hundreds separator
    }
    if (config.vfo[config.vfo_ix].step > 100000) {
        start_line -= 6; // skip thousands separator
    }

    display->writeRect(start_line, 17, start_line + 10, 17);
}

void FrequencyWidget::do_paint() {

    st_freqInfo freqInfo = {(unsigned long)radio::get_frequency(), config.vfo[config.vfo_ix].step, config.repeater_mode, radio::get_vfo()};

    if (this->dirty() || !(freqInfo == this->status)) {
        this->status = freqInfo;
        display->drawArea(&this->area, this);
        this->set_dirty();

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

bool FrequencyWidget::on_input(const st_inputEvent event) {
    switch (event.type) {

        case INPUT_EVENT_TYPE_TOUCH_END:
            view_manager::keypadView.set_value(radio::get_frequency(), 0, "Hz", "Frequency");
            view_manager::keypadView.on_changed = [](double v) { radio::set_frequency((uint64_t)v); };
            view_manager::push(&view_manager::keypadView);
            return true;
    }

    return false;
}
