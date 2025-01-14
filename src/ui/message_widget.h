//
// Created by Angel Dust on 29/06/2022.
//

#ifndef TRX_FRONTEND_MESSAGE_WIDGET_H
#define TRX_FRONTEND_MESSAGE_WIDGET_H

#include "widget.h"
#include "types.h"

#define MESSAGE_WIDGET_TITLE_MAX_LENGTH 100
#define MESSAGE_WIDGET_TEXT_MAX_LENGTH 256

class MessageWidget : public Widget {
public:

    MessageWidget(Rect parent_rect, Display *display,
                  FontDef *title_font, FontDef *text_font,
                  uint16_t border_color, uint16_t title_color, uint16_t text_color)
            : Widget(parent_rect, display), border_color{border_color}, title_font{title_font}, text_font{text_font},
              title_color{title_color}, text_color{text_color} {
    }

    bool on_input(const st_inputEvent event) override;

    void paint_callback() override;

    void set_title(const char *, uint16_t color);

    void set_msg(const char *);

protected:

    char title[MESSAGE_WIDGET_TITLE_MAX_LENGTH];
    char msg[MESSAGE_WIDGET_TEXT_MAX_LENGTH];
    int border_color;
    const FontDef *title_font;
    const FontDef *text_font;
    uint16_t title_color;
    const uint16_t text_color;

    void before_paint() override;
};

#endif //TRX_FRONTEND_MESSAGE_WIDGET_H
