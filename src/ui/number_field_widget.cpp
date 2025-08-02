//
// Created by Angel Dust on 16/06/2025.
//

#include "number_field_widget.h"
#include "Display_afb.h"
#include "utils.hpp"

NumberField::NumberField(Point parent_pos, int length, range_t range, int32_t step, char fill_char, bool can_loop)
    : Widget{{{parent_pos}, {8 * length, 16}}, &lcd}, range{range}, step{step}, length{length}, fill_char{fill_char}, can_loop{can_loop} {
}

int32_t NumberField::get_value() const {
    return value;
}

void NumberField::set_value(int32_t new_value, bool trigger_change) {
    if (can_loop) {
        if (new_value >= range.first) {
            new_value = new_value % (range.second + 1);
        } else {
            new_value = range.second + new_value + 1;
        }

        new_value = constrain(new_value, range.first, range.second);

        if (new_value != get_value()) {
            value = new_value;
            if (on_change && trigger_change) {
                on_change(value);
            }
            set_dirty();
        }
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
    char buf[length];
    format_long(value, buf, length);

    Color c = is_focused() ? C565_TEXT_FG_FOCUS : C565_TEXT_FG;

    display->gotoXY(0, 0);
    display->setColor(c);
    display->write(buf);

    return true;
}

void NumberField::before_paint() {
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
