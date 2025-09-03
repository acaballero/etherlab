//
// Created by Angel Dust on 13/07/2024.
//

#include "label_widget.h"
#include "input/inputEvent.h"
#include "ui/button_widget.h"
#include <math.h>

void Label::set_padding(uint16_t p) {
    padding = p;
}

uint16_t Label::get_padding() {
    return padding;
}

void Label::set_border_radius(bool top_left, bool top_right, bool bottom_right, bool bottom_left) {
    border_radius[0] = top_left;
    border_radius[1] = top_right;
    border_radius[2] = bottom_right;
    border_radius[3] = bottom_left;
}

bool Label::paint_callback() {

    display->clear(canvas_bg_color);
    display->setFont(font); // Note there's no guarantee paint_callback is called right after 'before_paint', so we set the font again
    display->setBgColor(bg_color);

    if (has_border) {
        display->setColor(bg_color);
        display->drawRoundedRectangle(0, 0, parent_rect().width(), parent_rect().height(), 3, style != LABEL_STYLE_HOLLOW, border_radius[0], border_radius[1],
                                      border_radius[3], border_radius[2]);
    }

    display->setColor(fg_color);

    uint16_t width = (lw + vw + uw);

    int16_t x;

    if (align == ALIGN_CENTER) {
        x = ceil((float)(parent_rect().width() - width) / 2.0);
    } else if (align == ALIGN_RIGHT) {
        x = parent_rect().width() - width - padding;
    } else {
        x = padding;
    }

    if (x < 0) {
        x = 0;
    }

    int16_t y;
    if (strchr(label, '\n')) { // two lines
        y = (parent_rect().height() - font->height * 2 - display->getVerticalLineSpacing()) / 2;
    } else {
        y = (parent_rect().height() - font->height + 2) / 2;
    }

    display->gotoXY(x, y);
    display->print(label, value, unit, fg_color, fg_color_value, fg_color_unit);

    return true;
}

bool Label::on_touch(const st_inputEvent) {

    if (on_select) {
        on_select(*this);
        return true;
    }

    return false;
}

ButtonStyle Label::get_style() const {
    return style;
}

void Label::set_style(ButtonStyle style) {
    Label::style = style;
}

void Label::before_paint() {
}

void Label::calc_widths() {
    display->setFont(font);
    lw = display->get_text_size(label).width();
    vw = display->get_text_size(value).width();
    uw = display->get_text_size(unit).width();
}

void Label::set_label(const char *t) {
    strncpy(label, t, MAX_CHARS);
    set_dirty();
    calc_widths();
}

char *Label::get_label() {
    return label;
}

void Label::set_value(const char *t) {
    strncpy(value, t, MAX_CHARS_VALUE);
    set_dirty();
    calc_widths();
}

void Label::set_unit(const char *t) {
    strncpy(unit, t, MAX_CHARS_UNIT);
    set_dirty();
    calc_widths();
}

void Label::set_color(uint16_t c) {
    set_color(c, fg_color_value, fg_color_unit);
}

void Label::set_color(uint16_t l, uint16_t v, uint16_t u) {
    fg_color = l;
    fg_color_value = v;
    fg_color_unit = u;
}

uint16_t Label::get_bg() const {
    return bg_color;
}

void Label::set_bg(uint16_t bg) {
    Label::bg_color = bg;
}

void Label::set_has_border(bool b) {
    has_border = b;
}

void Label::set_canvas_bg_color(uint16_t c) {
    canvas_bg_color = c;
}
