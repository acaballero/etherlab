//
// Created by Angel Dust on 13/07/2024.
//

#include "label_widget.h"

void Label::paint_callback() {

    display->clear();
    display->setColor(bg_color);
    display->drawRoundedRectangle(0, 0, area.width, area.height, 3, false);
    display->setColor(fg_color);
    display->setBgColor(bg_color);
    display->setFont(font);

    uint16_t lw = strlen(label);
    uint16_t vw = strlen(value);
    uint16_t uw = strlen(unit);
    uint16_t w = (lw + vw + uw);
    if (lw && vw) w++;
    if (uw) w++;

    int16_t width = w * (font->width + 1);
    int16_t x;

    if (align==ALIGN_CENTER) {
        x = (area.width - width) / 2;
    }
    else if (align==ALIGN_RIGHT) {
        x = area.width - width - display->get_padding_x();
    }
    else {
        x = display->get_padding_x();
    }

    if (x < 0) {
        x = 0;
    }

    display->gotoXY(x, (area.height - font->height + 2) / 2);
    display->print(label, value, unit, fg_color, fg_color_value, fg_color_unit);

}

void Label::do_paint() {
    if (this->dirty()) {
        display->drawArea(&this->area, this);
    }
}

void Label::set_label(const char *t) {
    strncpy(label, t, MAX_SIZE);
    set_dirty();
}

void Label::set_value(const char *t) {
    strncpy(value, t, MAX_SIZE);
    set_dirty();
}

void Label::set_unit(const char *t) {
    strncpy(unit, t, MAX_SIZE);
    set_dirty();
}

void Label::set_color(uint16_t c) {
    set_color(c, fg_color_value, fg_color_unit);
}

void Label::set_color(uint16_t l, uint16_t v, uint16_t u) {
    fg_color = l;
    fg_color_value = v;
    fg_color_unit = u;
}

