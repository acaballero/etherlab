//
// Created by Angel Dust on 13/07/2024.
//

#ifndef TRX_FRONTEND_LABEL_H
#define TRX_FRONTEND_LABEL_H

#include "Display_afb.h"
#include "button_widget.h"
#include "input/inputEvent.h"
#include "widget.h"
#include "types.h"
#include <stdint.h>

class Label : public Widget {
  public:
    static const size_t MAX_CHARS = 30;
    static const size_t MAX_CHARS_VALUE = 8;
    static const size_t MAX_CHARS_UNIT = 5;

    Label() {
        set_label("");
    }

    Label(Rect parent_rect) : Widget{parent_rect, &lcd} {
        set_label("");
    }

    Label(Rect parent_rect, Color fg_color, Color bg_color, ButtonStyle style = LABEL_STYLE_HOLLOW) : Widget{parent_rect, &lcd} {
        set_label("");
        set_color(fg_color);
        set_bg(bg_color);
        set_style(style);
    }

    Label(Point position, const char *text, Color fg_color) : Widget{} {

        int w = strlen(text) * font->width;
        int h = font->height + display->getVerticalLineSpacing() * 2;
        set_parent_rect({position, {w, h}});
        set_label(text);
        set_color(fg_color);
    }

    void paint_callback() override;

    void set_label(char const *);
    void set_value(char const *);
    void set_unit(char const *);

    char *get_label();

    void set_color(uint16_t);
    void set_color(uint16_t label, uint16_t text, uint16_t unit);

    ButtonStyle get_style() const;
    void set_style(ButtonStyle style);

    uint16_t get_bg() const;
    void set_bg(uint16_t bg);

    void set_has_border(bool);
    void set_border_radius(bool, bool, bool, bool);
    void set_canvas_bg_color(uint16_t);

    void set_padding(uint16_t p);
    uint16_t get_padding();

    std::function<void(Label &)> on_select;

  protected:
    char label[MAX_CHARS] = {};
    char value[MAX_CHARS_VALUE] = {};
    char unit[MAX_CHARS_UNIT] = {};
    uint16_t fg_color = C565_WHITE;
    uint16_t fg_color_value = C565_MAGENTA;
    uint16_t fg_color_unit = C565_GREY_LIGHT;
    uint16_t bg_color = C565_TRANSPARENT;
    uint16_t canvas_bg_color = 0;
    ButtonStyle style = LABEL_STYLE_HOLLOW;
    bool has_border = true;
    uint16_t padding = 4;
    bool border_radius[4] = {1, 1, 1, 1};

    void before_paint() override;

    bool on_touch(const st_inputEvent e) override;
};

#endif // TRX_FRONTEND_LABEL_H
