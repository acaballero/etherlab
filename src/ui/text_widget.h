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

    TextWidget(Rect parent_rect, const std::string &text_str, const std::string &name_str = {}, uint16_t fg_color = C565_TEXT_FG) : Widget(parent_rect, &lcd) {
        set_focusable(true);
        set_active(false);
        set_fg(fg_color);
        set_bg(C565_TRANSPARENT);
        set_text(text_str);
        name = name_str;
    };

    void set_text(const std::string &str);
    std::string &get_text() {
        return text;
    };

    TextWidget(const TextWidget &) = delete;
    TextWidget(TextWidget &&) = delete;
    TextWidget &operator=(const TextWidget &) = delete;
    TextWidget &operator=(TextWidget &&) = delete;

    bool paint_callback() override;

    bool on_input(const st_inputEvent event) override;
    void on_blur() override;

    void set_editable(bool b) {
        editable = b;
    }
    bool get_editalbe() {
        return editable;
    }

    void edit();

  protected:
    std::string text;
    std::string text_wrapped;
    std::string name{};
    int pos_x{0};

    bool editable = false;

    void before_paint() override;
};

#endif // TEXT_WIDGET_H
