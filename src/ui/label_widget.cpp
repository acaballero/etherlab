//
// Created by Angel Dust on 13/07/2024.
//

#include "label_widget.h"

void Label::paint_callback() {
    display->fill(0, 0, area.width, area.height, bg_color);

    display->setFont(font);

    int16_t width = strlen(text) * font->width;

    int16_t x = align_right ? area.width - width - 10 : 10;
    if (x < 0) {
        x = 0;
    }
    display->writeString(x, (area.height - font->height + 2) / 2, text, font, fg_color, bg_color);
}

void Label::do_paint() {
    if (this->dirty()) {
        display->drawArea(&this->area, this);
    }
}

void Label::set_text(const char *t) {
    strncpy(text, t, MAX_SIZE);
    set_dirty();
}

void Label::set_color(uint16_t c) {
    fg_color = c;
}

void Label::set_font(FontDef *f) {
    font = f;
}

void Label::set_align_right(bool b) {
    align_right = b;
}

