//
// Created by Angel Dust on 12/07/2024.
//

#include "button_widget.h"
#include <sys/_stdint.h>

void Button::set_text(char const *t) {
    strncpy(text, t, MAX_SIZE);
    set_dirty();
}

char *Button::get_text() { return text; }

void Button::set_two_lines(bool b) { two_lines = b; }

void Button::before_paint() {
    if (this->dirty()) {
        display->setFont(font);
    }
}

void Button::paint_callback() {

    uint16_t fg = fg_color, bg = bg_color;
    display->clear();

    if (!enabled()) {
        fg = fg_disabled_color, bg = bg_disabled_color;
    }

    else if (is_focused() || active()) {
        uint16_t tmp = fg;
        fg = bg;
        bg = tmp;
    }

    if (style == BUTTON_STYLE_3D) {
        display->writeRect(0, 0, parent_rect().width() - 1, 1, C565_GREY_LIGHT);
        display->writeRect(0, 0, 1, parent_rect().height() - 1, shadow);
        display->writeRect(parent_rect().width() - 2, 0, parent_rect().width() - 1, parent_rect().height() - 1, C565_GREY_LIGHT);
        display->writeRect(0, parent_rect().height() - 2, parent_rect().width() - 1, parent_rect().height() - 1, shadow);
        display->fill(1, 1, parent_rect().width() - 1, parent_rect().height() - 2, bg);
    } else {
        display->setColor(bg);
        display->drawRoundedRectangle(0, 0, parent_rect().width(), parent_rect().height(), 3, true);
    }

    display->setColor(fg);
    display->setBgColor(bg);
    display->setFont(font);

    uint16_t text_height = font->height;

    if (fn_writer) {
        display->gotoXY(display->get_padding_x(), (parent_rect().height() - text_height) / 2);
        fn_writer();
    } else {
        uint16_t lw = strlen(text);
        uint16_t vw = strlen(value);
        uint16_t uw = strlen(unit);

        if (two_lines) {

            uint16_t w = (vw + uw) * font->width;
            uint16_t xlabel = (parent_rect().width() - lw * font->width) / 2;
            uint16_t xval = (parent_rect().width() - w) / 2;
            uint16_t ylabel = (parent_rect().height() - (font->height + 1) * 2) / 2;
            uint16_t yval = ylabel + font->height + 3;
            display->gotoXY(xlabel, ylabel);
            display->print(text);
            display->gotoXY(xval, yval);
            display->print("", value, unit, fg, fg_color_value, fg_color_unit);

        } else {

            uint16_t w = (lw + vw + uw);
            if (uw) {
                w++;
            }

            int16_t width = w * (font->width);
            int16_t x;

            if (align == ALIGN_CENTER) {
                x = (parent_rect().width() - width) / 2;
            } else if (align == ALIGN_RIGHT) {
                x = parent_rect().width() - width - display->get_padding_x();
            } else {
                x = display->get_padding_x();
            }

            display->gotoXY(x, (parent_rect().height() - font->height + 2) / 2);
            display->print(text, value, unit, fg, fg_color_value, fg_color_unit);
        }
    }
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

uint16_t Button::get_fg() const { return fg_color; }

void Button::set_fg(uint16_t fg) { set_color(fg, fg_color_value, fg_color_unit); }

void Button::set_value(const char *t) {
    strncpy(value, t, MAX_SIZE);
    set_dirty();
}

void Button::set_unit(const char *t) {
    strncpy(unit, t, MAX_SIZE);
    set_dirty();
}

void Button::set_color(uint16_t l, uint16_t v, uint16_t u) {
    fg_color = l;
    fg_color_value = v;
    fg_color_unit = u;
}

uint16_t Button::get_bg() const { return bg_color; }

void Button::set_bg(uint16_t bg) { Button::bg_color = bg; }

uint16_t Button::get_shadow() const { return shadow; }

void Button::set_shadow(uint16_t shadow) { Button::shadow = shadow; }

FontDef *Button::get_font() const { return font; }

ButtonStyle Button::get_style() const { return style; }

void Button::set_style(ButtonStyle style) { Button::style = style; }
