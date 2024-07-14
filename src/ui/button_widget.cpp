//
// Created by Angel Dust on 12/07/2024.
//

#include "button_widget.h"

void Button::set_text(char const *t) {
    strncpy(text, t, MAX_SIZE);
    set_dirty();
}

char *Button::get_text() {
    return text;
}

void Button::do_paint() {
    if (this->dirty()) {
        display->setFont(font);
        display->drawArea(&this->area, this);
    }
}

void Button::paint_callback() {

    uint16_t fg = fg_color, bg = bg_color;

    if (is_focused() || active()) {
        uint16_t tmp = fg;
        fg = bg;
        bg = tmp;
    }

    display->writeRect(0, 0, area.width - 1, 1, C565_GREY_LIGHT);
    display->writeRect(0, 0, 1, area.height-1, shadow);
    display->writeRect(area.width - 2, 0, area.width - 1, area.height - 1, C565_GREY_LIGHT);
    display->writeRect(0, area.height - 2, area.width - 1, area.height-1, shadow);

    display->fill(1, 1, area.width - 1, area.height - 2, bg);

    uint16_t text_width = font->width * strlen(text);
    uint16_t text_height = font->height + 2;

    display->gotoXY((area.width - text_width) / 2,
                    (area.height - text_height) / 2);
    display->setColor(fg);
    display->setBgColor(bg);
    display->write(text);
}

void Button::on_focus() {
    if (on_highlight) {
        on_highlight(*this);
    }
}

bool Button::on_input(const st_inputEvent event) {

    if (event.type == INPUT_EVENT_TYPE_BUTTON_PRESS) {
        if (event.value) {
            if (on_select) {
                on_select(*this);
                return true;
            }
        }
    }

    switch (event.type) {
        case INPUT_EVENT_TYPE_TOUCH_START:
            set_active(true);
            set_dirty();
            return true;

        case INPUT_EVENT_TYPE_TOUCH_END:
            set_active(false);
            set_dirty();
            if (on_select) {
                on_select(*this);
            }
            return true;
        default:
            return false;
    }
}

uint16_t Button::get_fg() const {
    return fg_color;
}

void Button::set_fg(uint16_t fg) {
    Button::fg_color = fg;
}

uint16_t Button::get_bg() const {
    return bg_color;
}

void Button::set_bg(uint16_t bg) {
    Button::bg_color = bg;
}

uint16_t Button::get_shadow() const {
    return shadow;
}

void Button::set_shadow(uint16_t shadow) {
    Button::shadow = shadow;
}

FontDef *Button::get_font() const {
    return font;
}

void Button::set_font(FontDef *font) {
    Button::font = font;
}
