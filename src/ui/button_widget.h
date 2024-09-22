//
// Created by Angel Dust on 12/07/2024.
//

#ifndef TRX_FRONTEND_BUTTON_WIDGET_H
#define TRX_FRONTEND_BUTTON_WIDGET_H

#include <functional>
#include "widget.h"

class Button : public Widget {
public:

    static const size_t MAX_SIZE = 6;

    std::function<void(Button &)> on_select{};

    std::function<void(Button &)> on_highlight{};

    Button() : Widget() {};

    Button(Rect parent_rect, Display *display,const char *t, uint16_t fg_color) : Widget(parent_rect, display), fg_color{fg_color} {
        set_text(t);
    };

    void set_text(char const *value);

    char *get_text();

    void on_focus() override;

    bool on_input(const st_inputEvent event) override;

    uint16_t get_fg() const;

    void set_fg(uint16_t fg);

    uint16_t get_bg() const;

    void set_bg(uint16_t bg);

    uint16_t get_shadow() const;

    void set_shadow(uint16_t shadow);

    void paint_callback() override;

protected:
    char text[MAX_SIZE];
    uint16_t fg_color = C565_WHITE;
public:
    FontDef *get_font() const;

    void set_font(FontDef *font);

protected:
    uint16_t bg_color = C565_GREY_DARK;
    uint16_t shadow=C565_GREY_DARKER;
    FontDef *font = (FontDef *)&Font_11x18;
    void do_paint() override;
};

#endif //TRX_FRONTEND_BUTTON_WIDGET_H
