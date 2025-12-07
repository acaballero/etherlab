//
// Created by Angel Dust on 09/07/2025.
//

#ifndef TEXT_WIDGET_H
#define TEXT_WIDGET_H

#include "Display_afb.h"
#include "ui/lcd.h"
#include "widget.h"
#include <functional>
#include <stdint.h>

class TextWidget : public Widget {
  public:
    TextWidget() : Widget(){};

    TextWidget(Rect parent_rect, const std::string &t, uint16_t fg_color = C565_TEXT_FG) : Widget(parent_rect, &lcd) {
        set_focusable(true);
        set_fg(fg_color);
        set_bg(C565_TRANSPARENT);
        set_text(t);
    };

    void set_text(const std::string &str);
    void set_size(uint8_t size);

    TextWidget(const TextWidget &) = delete;
    TextWidget(TextWidget &&) = delete;
    TextWidget &operator=(const TextWidget &) = delete;
    TextWidget &operator=(TextWidget &&) = delete;

    bool paint_callback() override;

  protected:
    std::string text;

    void before_paint() override;
};

#endif // TEXT_WIDGET_H
