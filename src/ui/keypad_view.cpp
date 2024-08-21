//
// Created by Angel Dust on 12/07/2024.
//

#include "keypad_view.h"

bool KeypadView::on_input(const st_inputEvent event) {

    switch (event.type) {
        case INPUT_EVENT_TYPE_ENCODER:
            focused_button += event.value;
            if (focused_button < 0) {
                focused_button = sizeof(buttons) - 1;
            } else if (focused_button >= sizeof(buttons)) {
                focused_button = 0;
            }
            buttons[focused_button].set_focus(true);
            return true;
        case INPUT_EVENT_TYPE_TOUCH_START:

            return true;
    }

    return false;
}

void KeypadView::init() {

    const auto button_fn = [this](Button &button) {
        this->on_button(button);
    };

    label_widget.set_font((FontDef *) &Font_7x10);
    label_widget.set_color(C565_GREY_LIGHT);
    label_widget.set_align_right(true);
    add_child(&label_widget);

    add_child(&text_widget);

    const char *const key_caps = "123456789<0.";

    for (int n = 0; n < 12; n++) {
        Button *button = &buttons[n];
        char label[2]{key_caps[n], 0};

        add_child(button);

        button->id = n;
        button->on_highlight = [this](Button &button) {
            focused_button = button.id;
        };
        button->on_select = button_fn;
        button->set_parent_rect({
                                        (n % (cols - 1)) * button_w,
                                        (n / (cols - 1)) * button_h + button_h,
                                        button_w, button_h
                                });
        button->set_text(label);

    }

    add_children({&button_M,
                  &button_K,
                  &button_1,
                  &button_close
                 });

    button_M.on_select = button_fn;
    button_K.on_select = button_fn;
    button_1.on_select = button_fn;

    button_close.on_select = [this](Button &) {
        this->set_visible(false);
    };
}

void KeypadView::on_focus() {
    button_close.set_focus(true);
}

double KeypadView::value() const {
    return atof(buff);
}

void KeypadView::set_value(double new_value, uint8_t digits, const char *units, const char *label) {
    frac_digits = digits;

    char b[5];

    sprintf(b, "M%s", units);
    button_M.set_text(b);
    sprintf(b, "k%s", units);
    button_K.set_text(b);
    sprintf(b, "%s", units);
    button_1.set_text(b);

    ftoa(buff, MAX_DIGITS, new_value, frac_digits);

    label_widget.set_text(label);

    update_text();

    // Clear text next time
    index = 0;
}

void KeypadView::on_button(Button &button) {

    char *const s = button.get_text();

    if (&button == &button_M || &button == &button_K || &button == &button_1) {

        uint32_t multiplier = 1;

        if (&button == &button_M) {
            multiplier = 1000000;
        } else if (&button == &button_K) {
            multiplier = 1000;
        }

        float v = value();

        if (on_changed) {
            on_changed(v * multiplier);
        }

        this->set_visible(false);

    } else if (*s == '.') {

        int j;
        for (j = 0; j < index && buff[j] != '.'; j++);
        // append period if there are no period
        if (index == j && index < MAX_DIGITS) {
            buff[index++] = '.';
        }

    } else if (*s == '<') {
        if (index > 0) {
            index--;
        }

    } else {
        if (index < MAX_DIGITS) {
            buff[index++] = *s;
        }
    }

    buff[index] = 0;
    update_text();
}

void KeypadView::update_text() {
    text_widget.set_text(buff);
}

void KeypadView::do_paint() {

}

void KeypadView::with_multipliers(bool v) {
    show_multipliers = v;

    button_K.set_visible(show_multipliers);
    button_M.set_visible(show_multipliers);
}
