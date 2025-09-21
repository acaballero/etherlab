//
// Created by Angel Dust on 16/06/2025.
//

#include "number_field_widget.h"
#include "Display_afb.h"
#include "utils.hpp"

NumberField::NumberField(Point parent_pos, int l, range_t range, int32_t step, const char *u, bool can_loop)
    : Widget{{{parent_pos}, {}}, &lcd}, range{range}, step{step}, length{l}, can_loop{can_loop} {
    strncpy(units, u, sizeof(units));
    calc_size();
}

int32_t NumberField::get_value() const {
    return value;
}

void NumberField::calc_size() {
    set_width(font->width * (length + strlen(units)));
    set_height(font->height + 2);
}

void NumberField::set_value(int32_t new_value, bool trigger_change) {
    if (can_loop) {
        if (new_value >= range.first) {
            new_value = new_value % (range.second + 1);
        } else {
            new_value = range.second + new_value + 1;
        }
    }
    new_value = constrain(new_value, range.first, range.second);

    if (new_value != get_value()) {
        value = new_value;
        format_long(value, text, length);
        if (on_change && trigger_change) {
            on_change(value);
        }
        calc_size();
        set_dirty();
    }
}

void NumberField::set_range(const int32_t min, const int32_t max) {
    range.first = min;
    range.second = max;
    set_value(get_value(), false);
}

void NumberField::set_step(const int32_t new_step) {
    step = new_step;
}

bool NumberField::paint_callback() {

    bool was_trim_enabled = display->get_trim_enabled();
    display->set_trim_enabled(false);
    display->clear();
    int y = (parent_rect().height() - font->height) / 2;
    display->gotoXY(0, y);
    display->print(text);
    display->print(units);
    display->set_trim_enabled(was_trim_enabled);
    return true;
}

void NumberField::before_paint() {
    if (dirty()) {

        display->setColor(is_focused() ? C565_TEXT_FG_FOCUS : fg_color);
        display->setBgColor(bg_color);
    }
}

void NumberField::add(int32_t v) {
    int32_t old_value = get_value();
    set_value(get_value() + (v * step));

    if (on_wrap) {
        if ((v > 0) && (get_value() < old_value)) {
            on_wrap(1);
        } else if ((v < 0) && (get_value() > old_value)) {
            on_wrap(-1);
        }
    }
}

bool NumberField::on_input(const st_inputEvent event) {
    // if (key == KeyEvent::Select) {
    //     if (on_select) {
    //         on_select(*this);
    //         return true;
    //     }
    // }

    // // encoder

    //    add(delta);

    // // keys
    // if (key == 10) {
    //     if (on_select) {
    //         on_select(*this);
    //         return true;
    //     }
    // }
    // if (key == '+' || key == ' ') {
    //     return on_encoder(1);
    // }
    // if (key == '-' || key == 8) {
    //     return on_encoder(-1);
    // }
    // return false;

    // // touch

    // if (event.type == TouchEvent::Type::Start) {
    //     focus();
    // }
    //    return true;

    return false;
}
