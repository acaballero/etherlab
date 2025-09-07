//
// Created by Angel Dust on 29/06/2022.
//

#include "message_view.h"
#include "input/inputEvent.h"
#include "ui/console_widget.h"

void MessageView::init() {
    add_child(&console);
}

bool MessageView::on_input(const st_inputEvent e) {

    bool consumed = false;

    if (visible()) {
        switch (e.type) {

            case INPUT_EVENT_TYPE_TOUCH_START:
            case INPUT_EVENT_TYPE_BUTTON_RELEASE:
                consumed = true; // swallow
                break;

            default:
                set_visible(false);
                display->drawArea(&this->area, this);
                consumed = true;
                break;
        }
    }

    return consumed;
}

void MessageView::before_paint() {
    if (this->dirty()) {
        set_focus(true);
    }
}

void MessageView::clear() {
    console.clear();
}
void MessageView::on_show() {
    if (console.parent_rect().is_empty()) {
        console.set_parent_rect(parent_rect());
    }
}

void MessageView::add_msg(const char *header, const char *str) {
    std::string msg = ConsoleWidget::color_mark + std::string(1, (char)1) + header + ConsoleWidget::color_mark + std::string(1, (char)2) + str + "\n";
    console.write(msg);
}
