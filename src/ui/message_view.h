//
// Created by Angel Dust on 29/06/2022.
//

#ifndef TRX_FRONTEND_MESSAGE_WIDGET_H
#define TRX_FRONTEND_MESSAGE_WIDGET_H

#include "console_widget.h"
#include "view.h"
#include "types.h"

#define MESSAGE_WIDGET_TITLE_MAX_LENGTH 100
#define MESSAGE_WIDGET_TEXT_MAX_LENGTH 256

class MessageView : public View {
  public:
    MessageView(Rect parent_rect, FontDef *title_font, FontDef *text_font, uint16_t border_color, uint16_t title_color, uint16_t text_color)
        : View(parent_rect), border_color{border_color}, title_font{title_font}, text_font{text_font}, title_color{title_color}, text_color{text_color} {

        init();
    }

    bool on_input(const st_inputEvent event) override;

    void add_msg(const char *, const char *);

    void clear();

    void on_show() override;

  protected:
    int border_color;
    const FontDef *title_font;
    const FontDef *text_font;
    uint16_t title_color;
    const uint16_t text_color;

    ConsoleWidget console{{0, 0, parent_rect().width(), parent_rect().height()}, &lcd};

    void init();
    void before_paint() override;
};

#endif // TRX_FRONTEND_MESSAGE_WIDGET_H
