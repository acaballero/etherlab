//
// Created by Angel Dust on 12/07/2024.
//

#ifndef TRX_FRONTEND_BUTTON_WIDGET_H
#define TRX_FRONTEND_BUTTON_WIDGET_H

#include "widget.h"
#include <functional>

enum ButtonStyle { BUTTON_STYLE_3D = 0, BUTTON_STYLE_FLAT };

class Button : public Widget {
      public:
        static const size_t MAX_SIZE = 6;

        std::function<void(Button &)> on_select{};

        std::function<void(Button &)> on_highlight{};

        Button() : Widget(){};

        Button(Rect parent_rect, Display *display, const char *t, uint16_t fg_color) : Widget(parent_rect, display), fg_color{fg_color} { set_text(t); };

        void set_text(char const *value);

        char *get_text();

        void set_value(char const *);

        void set_unit(char const *);

        void on_focus() override;

        bool on_input(const st_inputEvent event) override;

        uint16_t get_fg() const;

        void set_fg(uint16_t fg);

        uint16_t get_bg() const;

        void set_color(uint16_t label, uint16_t text, uint16_t unit);

        void set_bg(uint16_t bg);

        uint16_t get_shadow() const;

        void set_shadow(uint16_t shadow);

        void paint_callback() override;

        ButtonStyle get_style() const;

        void set_style(ButtonStyle style);

        FontDef *get_font() const;

        std::function<void(void)> fn_writer; // Handler to delegate the writing

      protected:
        char text[MAX_SIZE];
        char value[MAX_SIZE];
        char unit[MAX_SIZE];
        uint16_t fg_color = C565_WHITE;
        uint16_t fg_disabled_color = C565_GREY_LIGHT;
        uint16_t fg_color_value = C565_BLUE;
        uint16_t fg_color_unit = C565_GREY_LIGHT;
        uint16_t bg_color = C565_GREY_DARK;
        uint16_t bg_disabled_color = C565_GREY_DARKER;
        uint16_t shadow = C565_GREY_DARKER;

        ButtonStyle style = BUTTON_STYLE_FLAT;

        void do_paint() override;
};

#endif // TRX_FRONTEND_BUTTON_WIDGET_H
