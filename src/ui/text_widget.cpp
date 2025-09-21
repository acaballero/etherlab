//
// Created by Angel Dust on 09/07/2025.
//

#include "text_widget.h"
#include "Display_afb.h"
#include "ui/widget.h"
#include <stdint.h>

void TextWidget::set_text(const std::string &t) {
    text = t;
    set_dirty();
}

void TextWidget::before_paint() {
    if (this->dirty()) {
        display->setFont(font);
    }
}

bool TextWidget::paint_callback() {

    uint16_t fg = fg_color, bg = bg_color;
    display->clear();

    display->setFont(font);

    int x = 0;
    if (align == ALIGN_CENTER) {
        x = (_parent_rect.width() - display->get_text_size(text).width()) / 2;
    }

    display->gotoXY(x, 0);
    display->setColor(fg);
    display->setBgColor(bg);
    display->print(text.c_str());

    return true;
}
