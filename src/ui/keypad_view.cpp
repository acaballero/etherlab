//
// Created by Angel Dust on 12/07/2024.
//

#include "keypad_view.h"
#include "Display_afb.h"
#include "ips_font.h"
#include "itemsTemplates.hpp"
#include "ui/button_widget.h"
#include "ui/widget.h"
#include "utils.hpp"

bool KeypadView::on_input(const st_inputEvent event) {

    bool consumed = Widget::on_input(event);

    if (consumed) {
        return consumed;
    }

    switch (event.type) {
        case INPUT_EVENT_TYPE_ENCODER:
            focused_button += event.value;
            if (focused_button < 0) {
                focused_button = sizeof(buttons) - 1;
            } else if (focused_button >= sizeof(buttons)) {
                focused_button = 0;
            }
            buttons[focused_button].set_focus(true);
            consumed = true;
            break;
        case INPUT_EVENT_TYPE_TOUCH_START:

            consumed = true;
            break;

        case INPUT_EVENT_TYPE_BUTTON_PRESS:
        case INPUT_EVENT_TYPE_BUTTON_DBL_PRESS:

            switch (event.value) {

                case KEY_BACK:
                    button_close.set_focus(true);
                    break;
                case FPANEL_DISPLAY_BUTTON_1:
                    if (show_multipliers) {
                        this->on_button(button_M);
                    } else {
                        this->on_button(button_1);
                    }
                    break;
                case FPANEL_DISPLAY_BUTTON_2:
                    if (show_multipliers) {
                        this->on_button(button_K);
                    }
                    break;
                case FPANEL_DISPLAY_BUTTON_3:
                    if (show_multipliers) {
                        this->on_button(button_1);
                    }
                    break;
                case FPANEL_DISPLAY_BUTTON_5:
                    del_char();
                    break;
                case FPANEL_DISPLAY_BUTTON_6:
                    this->set_visible(false);
                    break;
                default:
                    button_close.set_focus(true);
                    break;
            }

            consumed = true;
            break;
        default:
            consumed = false;
            break;
    }

    return consumed;
}

void KeypadView::init() {

    const auto button_fn = [this](Button &button, st_inputEvent) { this->on_button(button); };

    label_widget.set_font((FontDef *)&Font_7x10);
    label_widget.set_aling(ALIGN_CENTER);
    label_widget.set_border_radius(false, true, true, false);
    add_child(&label_widget);

    text_widget.set_font((FontDef *)&Font_11x18);

    text_widget.set_border_radius(true, false, false, true);
    text_widget.set_aling(ALIGN_RIGHT);

    add_child(&text_widget);

    const char *const key_caps = "123456789<0.";

    for (int n = 0; n < 12; n++) {
        Button *button = &buttons[n];
        char label[2]{key_caps[n], 0};

        add_child(button);

        button->id = n;
        button->on_highlight = [this](Button &button) { focused_button = button.id; };
        button->action = button_fn;
        button->set_style(BUTTON_STYLE_3D);
        button->set_aling(ALIGN_CENTER);
        button->set_parent_rect({(n % (cols - 1)) * button_w, (n / (cols - 1)) * button_h + button_h, button_w, button_h});
        button->set_text(label);
    }

    add_children({&button_M, &button_K, &button_1, &button_close, &display_panel_buttons});

    button_M.action = button_fn;
    button_K.action = button_fn;
    button_1.action = button_fn;

    button_close.action = [this](Button &, st_inputEvent) { this->set_visible(false); };

    display_panel_buttons.set_labels(show_multipliers ? display_buttons_labels : display_buttons_labels_no_mult);
}

void KeypadView::on_focus() { button_close.set_focus(true); }

double KeypadView::value() const {
    char b[MAX_DIGITS + 1];
    strncpy(b, buff, MAX_DIGITS + 1);
    char *endptr;
    removeChars(b, " ");
    double val = strtod(b, &endptr);
    return val;
}

void KeypadView::set_value(double new_value, uint8_t digits, const char *units, const char *label, double min, double max) {

    frac_digits = digits;
    if (!units) {
        units = "";
    }

    char b[5];

    sprintf(b, "M%s", units);
    button_M.set_text(b);
    sprintf(b, "k%s", units);
    button_K.set_text(b);
    sprintf(b, "%s", units[0] ? units : "x1");
    button_1.set_text(b);

    this->min = min;
    this->max = max;

    ftoa(buff, MAX_DIGITS, new_value, frac_digits);
    label_widget.set_label(label);

    update_text();

    // Clear text next time
    index = 0;
}

void KeypadView::del_char() {

    if (index > 0) {
        index--;
        buff[index] = 0;
        update_text();
        if (index > strlen(buff)) { // A thousand separator has been removed
            index--;
            buff[index] = 0;
            update_text();
        }
    }
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

        double v = value();

        if (min != max) {
            double v2 = constrain(v, min, max);
            if (v2 != v) {
                ftoa(buff, MAX_DIGITS, v2, frac_digits);
                v = v2;
            }
        }

        if (on_changed) {
            on_changed(v * multiplier);
        }

        this->set_visible(false);

    } else if (*s == decimal_separator) {

        int j;
        for (j = 0; j < index && buff[j] != decimal_separator; j++) {
            ;
        }
        // append period if there are no period
        if (index == j && index < MAX_DIGITS) {
            buff[index++] = decimal_separator;
        }

    } else if (*s == '<') {
        del_char();

    } else {
        if (index < MAX_DIGITS && (index <= frac_digits || buff[index - frac_digits - 1] != decimal_separator)) {
            buff[index++] = *s;
        }
    }

    buff[index] = '\0';
    update_text();
}

void KeypadView::update_text() {

    double v = value();

    double i;
    double fracPart = modf(v, &i);

    bool last_period = buff[strlen(buff) - 1] == decimal_separator;
    format_long(i, buff, 0, thousand_separator);

    if (fracPart > 0 || last_period) {

        sprintf(buff + strlen(buff), "%c", decimal_separator);
        if (fracPart > 0) {
            char fracStr[10];
            ftoa(fracStr, 10, fracPart, frac_digits);
            int l = strlen(fracStr);
            while (--l >= 0 && fracStr[l] == '0') {
                fracStr[l] = '\0';
            }
            sprintf(buff + strlen(buff), "%s", fracStr + 2);
        }
    }

    index = strlen(buff);

    text_widget.set_label(buff);
}

void KeypadView::before_paint() {}

void KeypadView::with_multipliers(bool v) {
    show_multipliers = v;

    button_K.set_visible(show_multipliers);
    button_M.set_visible(show_multipliers);

    display_panel_buttons.set_labels(show_multipliers ? display_buttons_labels : display_buttons_labels_no_mult);
}
