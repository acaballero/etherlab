//
// Created by Angel Dust on 12/07/2024.
//

#include "number_edit_view.h"
#include "Display_afb.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "itemsTemplates.hpp"
#include "ui/button_widget.h"
#include "ui/widget.h"
#include "utils.hpp"

bool NumberEditView::on_input(const st_inputEvent event) {

    bool consumed = Widget::on_input(event);

    if (consumed) {
        return consumed;
    }

    switch (event.type) {
        case INPUT_EVENT_TYPE_ENCODER:
            update_value(value + event.value * step);
            consumed = true;
            break;
        case INPUT_EVENT_TYPE_TOUCH_START:

            consumed = true;
            break;

        case INPUT_EVENT_TYPE_BUTTON_PRESS:
        case INPUT_EVENT_TYPE_BUTTON_DBL_PRESS:

            switch (event.value) {

                case FPANEL_DISPLAY_BUTTON_1:
                    this->on_button(buttons[DECR_BIG]);
                    break;
                case FPANEL_DISPLAY_BUTTON_2:
                    this->on_button(buttons[DECR]);
                    break;
                case FPANEL_DISPLAY_BUTTON_3:
                    this->on_button(buttons[INCR]);
                    break;
                case FPANEL_DISPLAY_BUTTON_4:
                    this->on_button(buttons[INCR_BIG]);
                    break;
                case FPANEL_DISPLAY_BUTTON_5:
                case BTN_ENCODER:
                case FPANEL_PAD_BUTTON_6:
                    this->on_button(buttons[OK]);
                    break;
                case KEY_BACK:
                default:
                case FPANEL_DISPLAY_BUTTON_6:
                    this->on_button(buttons[CANCEL]);
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

void NumberEditView::init() {

    set_focusable(true);
    const auto button_fn = [this](Button &button, st_inputEvent) {
        this->on_button(button);
    };

    title.set_font((FontDef *)&Font_7x10);
    title.set_aling(ALIGN_CENTER);

    add_child(&title);

    label_widget.set_aling(ALIGN_LEFT);
    label_widget.set_font((FontDef *)&Font_7x10);
    label_widget.set_border_radius(false, true, true, false);
    add_child(&label_widget);

    text_widget.set_font((FontDef *)&Font_11x18);

    text_widget.set_border_radius(true, false, false, true);
    text_widget.set_aling(ALIGN_RIGHT);

    add_child(&text_widget);

    for (Button &b : buttons) {
        b.action = button_fn;
        add_child(&b);
    }

    buttons[CANCEL].action = button_fn;

    display_panel_buttons.set_labels(display_buttons_labels);

    add_child(&display_panel_buttons);
}

void NumberEditView::on_focus() {
    buttons[CANCEL].set_focus(true);
}

void NumberEditView::update_value(double v) {

    value = constrain(v, min, max);

    char buff[max_length];
    format_double(value, buff, decimal_separator, thousand_separator, frac_digits, max_length);

    if (update_on_changes && on_changed) {
        on_changed(value);
    }

    text_widget.set_label(buff);
}

void NumberEditView::set_value(double new_value, uint8_t digits, const char *units, const char *label, double min, double max, double step, double step_big) {

    this->frac_digits = digits;
    this->step = step;
    this->step_big = step_big;
    if (!units) {
        units = "";
    }

    this->min = min;
    this->max = max;

    title.set_label(label);

    initial_value = new_value;
    update_value(new_value);

    char buf[20];
    if (units) {
        snprintf(buf, 20, "%s", units);
    }

    if (min != max) {
        char minbuff[10], maxbuff[10];
        if (min > 1000 || max > 1000) {
            char eng_min_units[3];
            char eng_max_units[3];
            format_eng(minbuff, min, "", eng_min_units, 2, false);
            format_eng(maxbuff, max, "", eng_max_units, 2, false);
            snprintf(buf + strlen(buf), 20 - strlen(buf), "\n[%s%s, %s%s]", minbuff, eng_min_units, maxbuff, eng_max_units);
        } else {
            snprintf(buf + strlen(buf), 20 - strlen(buf), "\n[%s, %s]", format_double(min, minbuff, '.', ' ', 2, true, 10),
                     format_double(max, maxbuff, '.', ' ', 2, true, 10));
        }
    }
    label_widget.set_label(buf);
}

void NumberEditView::on_button(Button &button) {

    switch (button.id) {
        case INCR_BIG:
            update_value(value + step_big);
            break;
        case INCR:
            update_value(value + step);
            break;
        case DECR:
            update_value(value - step);
            break;
        case DECR_BIG:
            update_value(value - step_big);
            break;
        case CANCEL:
        case OK:

            if (on_changed) {
                if (button.id == OK) {

                    on_changed(value);
                } else {
                    if (initial_value != value) {
                        on_changed(initial_value);
                    }
                }
            }

            this->set_visible(false);

            break;
    }
}

void NumberEditView::before_paint() {
}
