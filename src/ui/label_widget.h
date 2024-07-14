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

    Label() { set_text(""); }

    Label(Rect parent_rect) : Widget{parent_rect, &lcd} { set_text(""); }

    void paint_callback() override;

    void set_text(char const *);
    void set_font(FontDef *);
    void set_color(uint16_t);
    void set_align_right(bool);

protected:

    char text[MAX_SIZE];
    uint16_t fg_color = C565_BLACK;
    uint16_t bg_color = C565_WHITE;
    FontDef *font = (FontDef *) &Font_11x18;
    bool align_right = false;

    void do_paint() override;
};

#endif //TRX_FRONTEND_LABEL_H
