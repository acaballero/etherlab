//
// Created by Angel Dust on 15/01/2025.
//

#ifndef TRX_FRONTEND_FIELD_WIDGET_H
#define TRX_FRONTEND_FIELD_WIDGET_H

#include "Display_afb.h"
#include "ui/lcd.h"
#include "widget.h"
#include <functional>
#include <stdint.h>

/**
 * Editable field
 */
class Field : public Widget {
  public:
    static const size_t MAX_SIZE = 120;

    std::function<void(Field &)> on_select{};

    std::function<void(Field &)> on_highlight{};

    Field() : Widget(){};

    Field(Rect parent_rect, const char *t, uint16_t fg_color, uint8_t size = MAX_SIZE) : Widget(parent_rect, &lcd), fg_color{fg_color}, max_shown(size) {
        set_text(t);
    };

    void set_text(char const *value);
    void set_size(uint8_t size);

    char *get_text();

    Field(const Field &) = delete;
    Field(Field &&) = delete;
    Field &operator=(const Field &) = delete;
    Field &operator=(Field &&) = delete;

    void set_cursor(uint32_t pos);
    void set_inserting(bool b);
    void add_char(char c);
    void del_char();
    void set_max_shown(uint32_t n);

    void on_focus() override;

    bool on_input(const st_inputEvent event) override;

    uint16_t get_fg() const;

    void set_fg(uint16_t fg);

    uint16_t get_bg() const;

    void set_bg(uint16_t bg);

    void paint_callback() override;

    FontDef *get_font() const;

    std::function<void(void)> fn_writer; // Handler to delegate the writing

  protected:
    char text[MAX_SIZE];

    uint16_t fg_color = C565_GREY_DARK;
    uint16_t fg_disabled_color = C565_GREY_LIGHT;
    uint16_t fg_focused_color = C565_BLUE;
    uint16_t bg_color = C565_WHITE;
    uint16_t bg_disabled_color = C565_GREY_DARKER;
    uint16_t bg_focused_color = C565_WHITE;

    // Max characters at a time
    uint32_t max_shown = 30;
    uint32_t size = MAX_SIZE;
    uint32_t pos;
    bool inserting;

    void before_paint() override;
};

#endif // TRX_FRONTEND_FIELD_WIDGET_H
