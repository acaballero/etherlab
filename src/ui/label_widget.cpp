//
// Created by Angel Dust on 13/07/2024.
//

#include "label_widget.h"
#include "ui/button_widget.h"
#include <math.h>

void Label::paint_callback() {

    display->clear();

    display->setBgColor(bg_color);
    display->setFont(font);

    if (has_border) {
        display->setColor(bg_color);
        display->drawRoundedRectangle(0, 0, parent_rect().width(), parent_rect().height(), 3, style != LABEL_STYLE_HOLLOW);
    }

    display->setColor(fg_color);

    uint16_t lw = strlen(label);
    uint16_t vw = strlen(value);
    uint16_t uw = strlen(unit);
    uint16_t w = (lw + vw + uw);
    // if (lw && vw) w++;
    // if (uw) w++;

    int16_t width = w * (font->width);
    int16_t x;

    if (align == ALIGN_CENTER) {
        x = ceil((float)(parent_rect().width() - width) / 2.0);
    } else if (align == ALIGN_RIGHT) {
        x = parent_rect().width() - width - display->get_padding_x();
    } else {
        x = display->get_padding_x();
    }

    if (x < 0) {
        x = 0;
    }

    display->gotoXY(x, (parent_rect().height() - font->height + 2) / 2);
    display->print(label, value, unit, fg_color, fg_color_value, fg_color_unit);
}

ButtonStyle Label::get_style() const { return style; }

void Label::set_style(ButtonStyle style) { Label::style = style; }

void Label::before_paint() {}

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

void Label::set_color(uint16_t c) { set_color(c, fg_color_value, fg_color_unit); }

void Label::set_color(uint16_t l, uint16_t v, uint16_t u) {
    fg_color = l;
    fg_color_value = v;
    fg_color_unit = u;
}

uint16_t Label::get_bg() const { return bg_color; }

void Label::set_bg(uint16_t bg) { Label::bg_color = bg; }

void Label::set_has_border(bool b) { has_border = b; }
