//
// Created by Angel Dust on 12/07/2024.
//
#ifndef VIEW_MANAGER_H
#define VIEW_MANAGER_H

#include "ui/keyboard_view.h"
#include "ui/option_buttons_view.h"
#include "ui/number_edit_view.h"
#include "view.h"
#include "main_view.h"
#include "keypad_view.h"

namespace view_manager {
extern KeypadView keypadView;
extern KeyboardView keyboardView;
extern NumberEditView numberEditView;
extern OptionButtonsView optionButtonsView;
extern MainView mainView;
extern View *currentView;
void push(View *);
void init();
} // namespace view_manager

#endif // VIEW_MANAGER_H
