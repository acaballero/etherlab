//
// Created by Angel Dust on 09/07/2025.
//

#include "text_widget.h"
#include "Display_afb.h"
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

void TextWidget::paint_callback() {

    uint16_t fg = fg_color, bg = bg_color;
    display->clear();

    display->setFont(font);
    display->gotoXY(0, 0);
    display->setColor(fg);
    display->setBgColor(bg);
    display->print(text.c_str());
}

uint16_t TextWidget::get_fg() const {
    return fg_color;
}

void TextWidget::set_fg(uint16_t fg) {
    fg_color = fg;
}
