//
// Created by Angel Dust on 15/01/2025.
//

#include "field_widget.h"
#include "Display_afb.h"
#include "hw/stm32f4xx/gpio.h"
#include "input/inputEvent.h"
#include "itemsTemplates.hpp"
#include <stdint.h>

void Field::set_text(char const *t) {
    strncpy(text, t, size + 1);
    trim(text);
    set_dirty();
}

void Field::set_size(uint8_t s) { size = min2(MAX_SIZE, s); }

char *Field::get_text() { return text; }

void Field::before_paint() {
    if (this->dirty()) {
        display->setFont(font);
    }
}

void Field::paint_callback() {

    uint16_t fg = fg_color, bg = bg_color;
    display->clear();

    if (!enabled()) {
        fg = fg_disabled_color, bg = bg_disabled_color;
    }

    else if (is_focused() || active()) {

        fg = fg_focused_color;
        bg = bg_focused_color;
    }

    display->setFont(font);

    display->setColor(bg);

    display->drawRoundedRectangle(1, 0, parent_rect().width() - 1, parent_rect().height(), 3, true, true, false, true, false);

    int16_t x, y;

    x = 8;
    y = (parent_rect().height() - font->height + 2) / 2;

    auto offset = 0;
    uint8_t min_chars_after_cursor = 2;

    // First character to show
    if (pos >= max_shown - min_chars_after_cursor) {
        offset = pos - max_shown + 1 + min_chars_after_cursor;
    }

    display->gotoXY(x, y);
    display->setColor(fg);
    display->setBgColor(C565_TRANSPARENT);

    // Draw the text starting at the offset.
    for (uint32_t i = 0; i < max_shown; i++) {

        auto c = (i + offset < strlen(text)) ? text[i + offset] : ' ';
        display->writeChar(c);
    }

    int32_t cursor_x = font->width * (offset > 0 ? max_shown - 1 - min_chars_after_cursor : pos);

    // Invert the cursor character when in overwrite mode.
    if (!inserting && (pos < strlen(text))) {
        display->fill(x + cursor_x - 1, y - 2, x + cursor_x + font->width, y + font->height, fg);
        display->setColor(bg);
        display->setBgColor(fg);
        display->gotoXY(x + cursor_x, y);
        display->writeChar(text[pos]);
    }

    // Cursor
    display->fill(x + cursor_x - 1, y + font->height + 1, x + cursor_x + font->width, y + font->height + 2, C565_BLACK);
}

void Field::set_cursor(uint32_t new_pos) {
    pos = min2(new_pos, strlen(text));
    set_dirty();
}

void Field::set_inserting(bool b) {
    inserting = b;
    set_dirty();
}

void Field::set_max_shown(uint32_t n) {
    max_shown = n;
    set_dirty();
}

void Field::add_char(char c) {

    if ((strlen(text) >= size && inserting) || (pos >= strlen(text) && !inserting)) {
        return;
    }

    if (inserting) {

        for (int i = strlen(text); i >= (int)pos; i--) {
            text[i + 1] = text[i];
        }

        text[pos] = c;
    } else {
        text[pos] = c;
    }

    pos++;

    if (pos == strlen(text)) {
        set_inserting(true);
    }

    set_dirty();
}

void Field::del_char() {
    if (pos == 0) {
        return;
    }

    pos--;

    for (uint32_t i = pos; i < strlen(text); i++) {
        text[i] = text[i + 1];
    }

    set_dirty();
}

void Field::on_focus() {
    if (on_highlight) {
        on_highlight(*this);
    }
}

bool Field::on_input(const st_inputEvent event) {

    if (event.type == INPUT_EVENT_TYPE_BUTTON_PRESS) {
        if (event.value) {
            if (on_select) {
                on_select(*this);
                return true;
            }
        }
    }

    int32_t new_pos = pos + event.value;
    bool consumed = false;

    switch (event.type) {
        case INPUT_EVENT_TYPE_TOUCH_START:
            set_focus(true);
            set_dirty();
            consumed = true;
            break;

        case INPUT_EVENT_TYPE_ENCODER:

            // if (new_pos < 0) {
            //     new_pos = strlen(text); // wrap
            // } else if (new_pos > strlen(text)) {
            //     new_pos = 0;
            // }

            new_pos = constrain(new_pos, 0, (int32_t)strlen(text));

            set_cursor(new_pos);
            consumed = true;
            break;
        case INPUT_EVENT_TYPE_BUTTON_PRESS:
        case INPUT_EVENT_TYPE_BUTTON_DBL_PRESS:

            switch (event.value) {

                case KEY_BACK:
                    set_focus(false);

                    consumed = true;
                    break;
            }

            break;

        default:
            consumed = false;
            break;
    }

    return consumed;
}

uint16_t Field::get_fg() const { return fg_color; }

void Field::set_fg(uint16_t fg) { fg_color = fg; }

uint16_t Field::get_bg() const { return bg_color; }

void Field::set_bg(uint16_t bg) { Field::bg_color = bg; }

FontDef *Field::get_font() const { return font; }
