//
// Created by Angel Dust on 29/06/2022.
//

#include "message_widget.h"
#include "input/inputEvent.h"

void MessageWidget::paint_callback() {

    display->clear();

    if (!this->visible()) { // We clear this area once when it hides since not all its area is overwritten by other widgets
        return;
    }

    display->fillBuffer(C565_BLACK);
    display->setBgColor(C565_BLACK);
    display->setVerticalLineSpacing(6);
    display->setPadding(4, 4);

    display->writeRect(0, 0, area.box.width - 1, area.box.height - 1, border_color);
    display->fill(1, 1, area.box.width - 1, title_font->height + 3, border_color);

    uint8_t pad_x = display->get_padding_x();
    uint8_t pad_y = display->get_padding_y();

    display->gotoXY(pad_x, pad_y);
    display->setFont(title_font);
    display->setColor(title_color);
    display->setBgColor(border_color);
    display->print(title);

    display->gotoXY(pad_x, pad_y + title_font->height + display->getVerticalLineSpacing());
    display->setFont(text_font);
    display->setColor(text_color);
    display->setBgColor(C565_BLACK);
    display->print(msg);
}

bool MessageWidget::on_input(const st_inputEvent e) {

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

void MessageWidget::before_paint() {
    if (this->dirty()) {
        set_focus(true);
    }
}

void MessageWidget::set_title(const char *str, uint16_t color) {
    title_color = color;
    strncpy(title, str, MESSAGE_WIDGET_TITLE_MAX_LENGTH);
    set_dirty();
}

void MessageWidget::set_msg(const char *str) {
    strncpy(msg, str, MESSAGE_WIDGET_TITLE_MAX_LENGTH);
    set_dirty();
}
