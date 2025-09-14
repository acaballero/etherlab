//
// Created by Angel Dust on 29/06/2022.
//

#include "Display_afb.h"
#include "input/inputEvent.h"
#include "ui/console_widget.h"
#include "message_view.h"

void MessageView::init() {
    set_border_width(2);
    console.set_bg(C565_DARKEST);
    console.set_padding(4, 4);
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
    st_datetime datetime = rtc_get_date_time();

    char buff[11];
    sprintf(buff, "[%02d:%02d:%02d] ", datetime.time.Hours, datetime.time.Minutes, datetime.time.Seconds);
    std::string msg = ConsoleWidget::color_mark + std::string(1, (char)1) + buff + ConsoleWidget::color_mark + std::string(1, (char)2) + header + ": " +
                      ConsoleWidget::color_mark + std::string(1, (char)1) + str + "\n";
    console.write(msg);
}
