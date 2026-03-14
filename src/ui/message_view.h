//
// Created by Angel Dust on 29/06/2022.
//

#ifndef TRX_FRONTEND_MESSAGE_WIDGET_H
#define TRX_FRONTEND_MESSAGE_WIDGET_H

#include "console_widget.h"
#include "label_widget.h"
#include "menu_options.h"
#include "text_widget.h"
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

    void add_log(const char *label, const char *msg);

    void show_msg(const char *header, const char *msg);

    void clear_log();

    void on_show() override;

  protected:
    static constexpr uint8_t border_width = 1;
    static constexpr uint8_t shadow_width = 2;
    static constexpr uint8_t padding = shadow_width + border_width + 0;
    static constexpr uint8_t title_height = 20;

    int border_color;
    const FontDef *title_font;
    const FontDef *text_font;
    uint16_t title_color;
    const uint16_t text_color;

    ConsoleWidget console{{padding, padding + title_height, parent_rect().width() - (padding)*2, parent_rect().height() - (padding)*2},
                        Widget::default_display()};

    TextWidget text_w{};
    Label title_w{{}, C565_WHITE, C565_GREY_DARKER, ButtonStyle::BUTTON_STYLE_FLAT};

    void init();
    void before_paint() override;

    Menu::menu_action_st menu_actions[1] = {{"Close", [this]() {
                                                 set_visible(false);
                                             }}};

    Menu::menu_actions_st actions = {menu_actions, sizeof(menu_actions) / sizeof(Menu::menu_action_st)};

    Menu::menu_actions_st *get_quick_actions() override {
        return &actions;
    }
};

#endif // TRX_FRONTEND_MESSAGE_WIDGET_H
