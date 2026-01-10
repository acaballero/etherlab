//
// Created by Angel Dust on 09/07/2025.
//

#include "text_widget.h"
#include "Display_afb.h"
#include "ui/widget.h"
#include "ui/view_manager.h"
#include <stdint.h>

void TextWidget::set_text(const std::string &t) {

    display->setFont(font);
    text = t;
    text_wrapped = display->fit_text(text, parent_rect().width(), parent_rect().height());

    if (align == ALIGN_CENTER) {
        pos_x = (_parent_rect.width() - display->get_text_size(text).width()) / 2;
    }
    set_dirty();
}

void TextWidget::before_paint() {
    if (this->dirty()) {
        display->set_trim_enabled(false);
        display->setFont(font);
    }
}

void TextWidget::on_blur() {
    set_active(false);
}

bool TextWidget::paint_callback() {

    display->clear();
    display->setFont(font);
    display->gotoXY(pos_x, 0);
    display->setColor(active() ? get_fg() : is_focused() ? C565_TEXT_FG_FOCUS : C565_TEXT_FG);
    display->setBgColor(get_bg());
    display->print(text_wrapped.c_str());

    return true;
}

void TextWidget::edit() {
    view_manager::keyboardView.set_text(text.c_str());
    view_manager::keyboardView.set_label(!name.empty() ? name.c_str() : "Edit");
    view_manager::keyboardView.set_size(255);
    view_manager::keyboardView.on_changed = [this](char *str) {
        text = str;
    };
    view_manager::push((View *)&view_manager::keyboardView);
};

bool TextWidget::on_input(const st_inputEvent event) {

    if (!editable) {
        return false;
    }

    bool consumed = false;

    switch (event.type) {

        case INPUT_EVENT_TYPE_TOUCH_START:

            set_focus(true);

            consumed = true;
            break;
        case INPUT_EVENT_TYPE_TOUCH_END:

            edit();
            consumed = true;
            break;

        case INPUT_EVENT_TYPE_BUTTON_PRESS:
            switch (event.value) {
                case KEY_BACK:
                case BTN_ENCODER:
                case FPANEL_PAD_BUTTON_6:

                    edit();
                    set_dirty();
                    consumed = true;

                    break;

                default:
                    consumed = false;
                    break;
            }

            break;

        default:
            consumed = false;
            break;
    }

    return consumed;
}
