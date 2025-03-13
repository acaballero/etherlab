//
// Created by Angel Dust on 12/07/2024.
//

#ifndef TRX_FRONTEND_BUTTON_WIDGET_H
#define TRX_FRONTEND_BUTTON_WIDGET_H

#include "Display_afb.h"
#include "input/inputEvent.h"
#include "widget.h"
#include <functional>

enum ButtonStyle { BUTTON_STYLE_3D = 0, BUTTON_STYLE_FLAT, LABEL_STYLE_HOLLOW };

class Button : public Widget {
  public:
    static const size_t MAX_CHARS = 16;
    static const size_t MAX_CHARS_VALUE = 8;
    static const size_t MAX_CHARS_UNIT = 4;

    std::function<void(Button &, st_inputEvent)> action{};
    std::function<void(Button &)> on_highlight{};

    Button() : Widget(){};

    Button(Rect parent_rect, Display *display, const char *t, uint16_t fg_color = C565_BLACK, uint16_t bg_color = C565_GREY_LIGHT,
           ButtonStyle style = BUTTON_STYLE_FLAT, Align aling = ALIGN_LEFT, uint32_t id = 0)
        : Widget(parent_rect, display), fg_color{fg_color}, bg_color{bg_color}, style(style) {
        set_aling(aling);
        set_text(t);
        this->id = id;
    };

    void set_text(char const *value);

    char *get_text();

    void set_value(char const *);

    void set_unit(char const *);

    void on_focus() override;

    void on_blur() override;

    bool on_input(const st_inputEvent event) override;

    uint16_t get_fg() const;

    void set_fg(uint16_t fg);

    uint16_t get_bg() const;

    void set_color(uint16_t label, uint16_t text, uint16_t unit);

    void set_bg(uint16_t bg);

    void set_text_bg(uint16_t bg);

    uint16_t get_shadow() const;

    void set_shadow(uint16_t shadow);

    // START Painter overrides
    void paint_callback() override;

    // END Painter overrides

    ButtonStyle get_style() const;

    void set_style(ButtonStyle style);

    FontDef *get_font() const;

    void set_two_lines(bool b);

    void set_dimmed(bool);

    std::function<void(void)> fn_writer; // Handler to delegate the writing

  protected:
    char text[MAX_CHARS];
    char value[MAX_CHARS_VALUE]{""};
    char unit[MAX_CHARS_UNIT]{""};
    bool two_lines = false;

    int fd = 1;

    uint16_t fg_color = C565_DARKEST;
    uint16_t text_bg_color = C565_TRANSPARENT;
    uint16_t fg_disabled_color = C565_BLACK;
    uint16_t fg_color_value = C565_BLUE;
    uint16_t fg_color_unit = C565_GREY_LIGHT;
    uint16_t fg_color_focused = C565_BLACK;
    uint16_t fg_dimmed_color = C565_GREY_LIGHT;
    uint16_t bg_color = C565_GREY_LIGHT;
    uint16_t bg_color_focused = C565_WHITE;
    uint16_t bg_disabled_color = C565_GREY_DARKER;
    uint16_t bg_dimmed_color = C565_GREY_DARKER;
    uint16_t shadow_light = C565_WHITE;
    uint16_t shadow = C565_GREY_DARKER;

    bool dimmed = false;

    ButtonStyle style = BUTTON_STYLE_FLAT;

    void before_paint() override;
};

#endif // TRX_FRONTEND_BUTTON_WIDGET_H
