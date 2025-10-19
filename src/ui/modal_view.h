//
// Created by Angel Dust on 09/07/2025.
//

#ifndef MODAL_VIEW_H
#define MODAL_VIEW_H

#include "Display_afb.h"
#include "label_widget.h"
#include "menu_actions.h"
#include "menu_options.h"
#include "text_widget.h"
#include "view.h"
#include "button_widget.h"

enum modal_t { INFO = 0, YESNO, ABORT };

class ModalView : public View {
  public:
    ModalView(const std::string &title, const std::string &message, modal_t type, std::function<void(bool)> on_select);

    static std::unique_ptr<ModalView> info(const std::string &title, const std::string &message) {
        return std::make_unique<ModalView>(title, message, INFO, nullptr);
    }

    void on_focus() override;

    void before_paint() override;

    Menu::menu_actions_st *get_quick_actions() override {
        return &quick_actions;
    };

  private:
    const std::string message;
    const modal_t type;
    const std::function<void(bool)> on_select{nullptr};

    TextWidget text_w{};

    Label title_w{{}, C565_WHITE, C565_GREY_DARKER, ButtonStyle::BUTTON_STYLE_FLAT};

    Button button_ok{
        {},
        &lcd,
        "OK",
    };

    Button button_yes{
        {},
        &lcd,
        "YES",
    };

    Button button_no{
        {},
        &lcd,
        "NO",
    };

    Menu::menu_actions_st quick_actions = {Menu::get_navigation_actions().actions, 4};
};

#endif
