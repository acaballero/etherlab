//
// Created by Angel Dust on 13/07/2024.
//

#ifndef TRX_FRONTEND_LABEL_H
#define TRX_FRONTEND_LABEL_H

#include "widget.h"
#include "types.h"

class Label : public Widget {
public:

    static const uint8_t MAX_SIZE = 20;

    Label() { set_label(""); }

    Label(Rect parent_rect) : Widget{parent_rect, &lcd} { set_label(""); }

    void paint_callback() override;

    void set_label(char const *);
    void set_value(char const *);
    void set_unit(char const *);

    void set_color(uint16_t);
    void set_color(uint16_t label,uint16_t text,uint16_t unit);

protected:

    char label[MAX_SIZE];
    char value[MAX_SIZE];
    char unit[MAX_SIZE];
    uint16_t fg_color = C565_WHITE;
    uint16_t fg_color_value = C565_BLUE;
    uint16_t fg_color_unit = C565_GREY_LIGHT;
    uint16_t bg_color = C565_TRANSPARENT;

    void do_paint() override;
};

#endif //TRX_FRONTEND_LABEL_H
