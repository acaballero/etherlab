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

bool FrequencyWidget::paint_callback() {

    char buf[20];

    uint16_t fg_color;

    display->set_trim_enabled(true);
    display->clear();

    format_long(radio::get_frequency(), buf);

    display->setBgColor(C565_TRANSPARENT);

    if (changing_step) {
        fg_color = C565_FIELD_FG;
    } else {
        if (scanner::scanner_config.status == scanner::SCANNER_STATUS_RUNNING) {
            fg_color = C565_MAGENTA;
        } else {
            fg_color = C565_YELLOW;
        }
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

    Widget::paint_callback();

    return true;
}

void FrequencyWidget::before_paint() {
    st_freqInfo freqInfo = {(unsigned long)radio::get_frequency(),
                            config.vfo[config.vfo_ix].step,
                            config.repeater_mode,
                            radio::get_vfo(),
                            freq_memory::memory_mode_on(),
                            config.mode};

    if (this->dirty() || !(freqInfo == status)) {
        this->set_dirty();
    }
}

bool FrequencyWidget::on_input(st_inputEvent e) {
    bool consumed = false;

    if (e.is_touch()) {
        return on_touch(e);
    }

    switch (e.type) {

        case INPUT_EVENT_TYPE_BUTTON_PRESS:

            if (e.value == BTN_ENCODER and e.ms < LONG_PRESS_MS) {
                changing_step = !changing_step;
                set_dirty();
                consumed = true;
            }
            break;
        case INPUT_EVENT_TYPE_ENCODER:

            if (changing_step) { // with push button low, change the step size instead of frequency
                radio::change_step(-e.value);
            } else {
                radio::change_frequency(e.value);
            }
            consumed = true;

            break;
        default:
            break;
    }

    return consumed;
}

bool FrequencyWidget::on_touch(const st_inputEvent e) {

    if (e.ms > LONG_PRESS_MS) {
        freq_memory::open_save_current();

    } else {
        Menu::open_keypad<uint64_t>(
            radio::get_frequency(), "Hz", "Frequency", 6, true,
            [](uint64_t v) {
                radio::set_frequency((uint64_t)v);
            },
            0, 0);
    }
    return true;
}
