//
// Created by Angel Dust on 03/02/2025.
//

#ifndef TRX_FRONTEND_MENU_OPTIONS_H
#define TRX_FRONTEND_MENU_OPTIONS_H

#include "radio.h"
#include "stdio.h"
#include "Display_afb.h"
#include "types.h"
#include "ui/ui_types.h"
#include <functional>

namespace Menu {

enum MenuStatus { ACTIVE, IDLE, UNKNOWN };

template <typename T> struct menu_option_st {
    const char *name;
    T value;
    uint16_t fg_color = C565_TEXT_FG;
    uint16_t bg_color = C565_TEXT_BG;
    bool enabled = true;
};

struct menu_action_st {
    std::string name;
    std::function<void(void)> action;
    uint16_t fg_color = C565_TEXT_FG;
    uint16_t bg_color = C565_TEXT_BG;
    std::function<void(void)> fn_writer{nullptr};
    bool enabled{true};
};

struct menu_actions_st {
    menu_action_st *actions;
    size_t size;
};

template <typename T> using menu_options_t = menu_option_st<T> *;

extern menu_option_st<uint16_t> color_options[23];
extern menu_option_st<MODULATION_MODE> modulation_options[6];
extern menu_option_st<radio::BAND> band_options[radio::BAND_NONE + 1];
extern menu_option_st<radio::IF_FILTER> if_filter_options[9];

} // namespace Menu

#endif
