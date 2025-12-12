//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_MENU_WIDGET_H
#define TRX_FRONTEND_MENU_WIDGET_H

#include "ui/menu_actions.h"
#include "widget.h"
#include "types.h"

class MenuWidget : public Widget {
  public:
    MenuWidget(Rect r) : Widget(r, &lcd) {
        set_focusable(true);
        set_default_quick_actions();
    };

    bool paint_callback() override;

    bool on_input(const st_inputEvent event) override;

    Menu::menu_actions_st *get_quick_actions() override {
        return current_actions;
    }

    void set_quick_actions(Menu::menu_actions_st *actions) {
        current_actions = actions;
    }

    void set_default_quick_actions() {
        set_quick_actions(&Menu::get_navigation_actions());
    }

  protected:
    void before_paint() override;
    Menu::menu_actions_st *current_actions;
};

#endif // TRX_FRONTEND_MENU_WIDGET_H
