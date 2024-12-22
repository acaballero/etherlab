//
// Created by Angel Dust on 12/07/2024.
//

#include "button_widget.h"

void Button::set_text(char const *t) {
        strncpy(text, t, MAX_SIZE);
        set_dirty();
}

char *Button::get_text() { return text; }

void Button::do_paint() {
        if (this->dirty()) {
                display->setFont(font);
                display->drawArea(&this->area, this);
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
                display->writeRect(0, 0, area.width - 1, 1, C565_GREY_LIGHT);
                display->writeRect(0, 0, 1, area.height - 1, shadow);
                display->writeRect(area.width - 2, 0, area.width - 1, area.height - 1, C565_GREY_LIGHT);
                display->writeRect(0, area.height - 2, area.width - 1, area.height - 1, shadow);
                display->fill(1, 1, area.width - 1, area.height - 2, bg);
        } else {
                display->setColor(bg);
                display->drawRoundedRectangle(0, 0, area.width, area.height, 3, true);
        }

        display->setColor(fg);
        display->setBgColor(bg);
        display->setFont(font);

        uint16_t text_height = font->height;

        if (fn_writer) {
                display->gotoXY(display->get_padding_x(), (area.height - text_height) / 2);
                fn_writer();
        } else {
                uint16_t lw = strlen(text);
                uint16_t vw = strlen(value);
                uint16_t uw = strlen(unit);
                uint16_t w = (lw + vw + uw);
                if (uw)
                        w++;

                int16_t width = w * (font->width);
                int16_t x;

                if (align == ALIGN_CENTER) {
                        x = (area.width - width) / 2;
                } else if (align == ALIGN_RIGHT) {
                        x = area.width - width - display->get_padding_x();
                } else {
                        x = display->get_padding_x();
                }

                display->gotoXY(x, (area.height - font->height + 2) / 2);
                display->print(text, value, unit, fg, fg_color_value, fg_color_unit);
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
