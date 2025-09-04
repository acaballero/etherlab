//
// Created by Angel Dust on 12/07/2024.
//

#include "button_widget.h"
#include "input/inputEvent.h"
#include "ui/ui_types.h"
#include <stdint.h>

void Button::set_text(char const *t) {
    strncpy(text, t, MAX_CHARS);
    set_dirty();
    calc_widths();
}

char *Button::get_text() {
    return text;
}

void Button::set_two_lines(bool b) {
    two_lines = b;
}

void Button::calc_widths() {
    display->setFont(font);
    lw = display->get_text_size(text).width();
    vw = display->get_text_size(value).width();
    uw = display->get_text_size(unit).width();
    if (variable_width) {
        set_width();
    }
}

void Button::draw_box(int box_width, uint16_t bg) {

    uint16_t fg = display->getColor();

    if (style == BUTTON_STYLE_3D) {
        display->writeRect(0, 0, box_width - 1, 1, shadow_light);
        display->writeRect(0, 0, 1, parent_rect().height() - 1, shadow);
        display->writeRect(box_width - 2, 0, box_width - 1, parent_rect().height() - 1, shadow_light);
        display->writeRect(0, parent_rect().height() - 2, box_width - 1, parent_rect().height() - 1, shadow);
        display->fill(1, 1, box_width - 1, parent_rect().height() - 2, bg);
    } else {
        display->clear();
        display->setColor(bg);
        display->drawRoundedRectangle(0, 0, box_width, parent_rect().height(), 3, true);
    }

    display->setColor(fg);
}

void Button::set_width() {

    Rect r = parent_rect();

    if (two_lines) {
        uint16_t line1w = lw;
        uint16_t line2w = (vw + uw);
        r.set_width(max2(line1w, line2w) + (2 * display->get_padding_x()));
    } else {
        uint16_t w = (lw + vw + uw);
        r.set_width(w + (2 * display->get_padding_x()));
    }

    set_parent_rect(r);
}

void Button::before_paint() {
}

bool Button::paint_callback() {

    uint16_t fg = fg_color, bg = bg_color;

    if (!enabled()) {
        fg = fg_disabled_color, bg = bg_disabled_color;
    } else if (dimmed) {
        fg = fg_dimmed_color;
        bg = bg_dimmed_color;
    } else if (is_focused() || active()) {
        fg = fg_color_focused;
        bg = bg_color_focused;
    }

    display->setColor(fg);
    display->setBgColor(text_bg_color);
    display->setFont(font);

    uint16_t text_height = font->height;
    uint16_t box_width = parent_rect().width();

    if (fn_writer) {
        draw_box(box_width, bg);
        display->gotoXY(display->get_padding_x(), (parent_rect().height() - text_height) / 2);
        fn_writer();
    } else {

        if (two_lines) {

            uint16_t line2w = (vw + uw);

            uint16_t xlabel = (box_width - lw) >> 1;
            uint16_t xval = (box_width - line2w) >> 1;
            uint16_t ylabel = (parent_rect().height() - ((font->height + 1) << 1)) >> 1;
            uint16_t yval = ylabel + font->height + 3;

            draw_box(box_width, bg);

            display->gotoXY(xlabel, ylabel);
            display->print(text);
            if (vw) {
                display->gotoXY(xval, yval);
                display->print("", value, unit, fg, fg_color_value, fg_color_unit);
            }

        } else {

            uint16_t w = (lw + vw + uw);

            int16_t x;

            if (align == ALIGN_CENTER) {
                x = (box_width - w + 1) >> 1;
            } else if (align == ALIGN_RIGHT) {
                x = box_width - w - display->get_padding_x();
            } else {
                x = display->get_padding_x();
            }

            draw_box(box_width, bg);

            display->gotoXY(x, (parent_rect().height() - font->height + 2) >> 1);
            if (vw) {
                display->print(text, value, unit, fg, fg_color_value, fg_color_unit);
            } else {
                display->print(text);
            }
        }
    }

    return true;
}

void Button::on_blur() {
    // Paint immediatelly
    paint();
    set_clean();
}

void Button::on_focus() {

    // printf_("Button %s on_focus\n", this->text);

    if (on_highlight) {
        on_highlight(*this);
    }
    // Paint immediatelly
    paint();
    set_clean();
}

bool Button::on_input(const st_inputEvent event) {

    // printf_("Button %s on_input %s\n", this->text, event.type == INPUT_EVENT_TYPE_BUTTON_PRESS ? "press" : "release");

    if (event.type == INPUT_EVENT_TYPE_BUTTON_PRESS || event.type == INPUT_EVENT_TYPE_BUTTON_DBL_PRESS) {
        if (event.value && event.value == BTN_ENCODER) {
            if (action) {
                action(*this, event);
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
            if (action) {
                action(*this, event);
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
    set_color(fg, fg_color_value, fg_color_unit);
}

void Button::set_value(const char *t) {
    strncpy(value, t, MAX_CHARS_VALUE);
    set_dirty();
    calc_widths();
}

void Button::set_unit(const char *t) {
    strncpy(unit, t, 4);
    set_dirty();
    calc_widths();
}

void Button::set_color(uint16_t l, uint16_t v, uint16_t u) {
    fg_color = l;
    fg_color_value = v;
    fg_color_unit = u;
}

void Button::set_dimmed(bool b) {
    dimmed = b;
};

uint16_t Button::get_bg() const {
    return bg_color;
}

void Button::set_text_bg(uint16_t c) {
    text_bg_color = c;
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

ButtonStyle Button::get_style() const {
    return style;
}

void Button::set_style(ButtonStyle style) {
    Button::style = style;
}
