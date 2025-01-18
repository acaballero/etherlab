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

    // TODO: Views containing child widgets that also render themselves like this have to clear the buffer at every pass. This causes flickr because
    // first, the view is rendered, and then their children. One way to prevent this can be rendering just areas where there's no children.
    display->clear();

    if (config.repeater_mode != radio::RPT_MODE_OFF) {

        sprintf(buf, "%d", config.repeater_offset / 1000);

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

    format_long(radio::get_frequency(), buf2);
    sprintf(buf, "%14s", buf2);

    display->setBgColor(C565_TRANSPARENT);

    if (scanner::scanner_config.status == scanner::SCANNER_STATUS_RUNNING) {
        fg_color = C565_MAGENTA;
    } else {
        fg_color = C565_YELLOW;
    }

    uint16_t x = (area.width / 4) + 24 + 4;

    FontDef *font = (FontDef *)&Font_11x18;
    display->writeString(x, 0, buf, font, fg_color, C565_TRANSPARENT);
    display->setColor(C565_GREY_LIGHT);

    uint8_t dec_place = (uint8_t)log10((double)config.vfo[config.vfo_ix].step) + 1;
    uint16_t start_line = area.width - (dec_place * font->width);

    if (dec_place > 3) {
        start_line -= font->width - font->trim_punct_end - font->trim_punct_start; // sip hundreds separator
    }
    if (dec_place > 6) {
        start_line -= font->width - font->trim_punct_end - font->trim_punct_start; // skip thousands separator
    }

    display->writeRect(start_line + 2, 16, start_line + 2, 17);
    display->writeRect(start_line + 1, 17, start_line + 3, 17);
    display->writeRect(start_line, 18, start_line + 4, 18);

    for (const auto child : this->children()) {
        display->setOffset(child->screen_rect().left(), child->screen_rect().top() - 1, child->screen_rect().width() - 1, child->screen_rect().height() - 1);
        child->paint_callback();
        display->clearOffset();
    }
}

void FrequencyWidget::before_paint() {

    st_freqInfo freqInfo = {(unsigned long)radio::get_frequency(), config.vfo[config.vfo_ix].step, config.repeater_mode, radio::get_vfo()};

    if (this->dirty() || !(freqInfo == this->status)) {
        this->status = freqInfo;
        this->set_dirty();
    }
}

bool FrequencyWidget::on_input(const st_inputEvent event) {
    switch (event.type) {

        case INPUT_EVENT_TYPE_TOUCH_END:
            view_manager::keypadView.set_value(radio::get_frequency(), 0, "Hz", "Frequency");
            view_manager::keypadView.on_changed = [](double v) { radio::set_frequency((uint64_t)v); };
            view_manager::push(&view_manager::keypadView);
            return true;
        default:
            return false;
    }

    return false;
}
