//
// Created by Angel Dust on 12/07/2024.
//
#ifndef VIEW_MANAGER_H
#define VIEW_MANAGER_H

#include "input/inputEvent.h"
#include "ui/keyboard_view.h"
#include "ui/option_buttons_view.h"
#include "ui/number_edit_view.h"
#include "view.h"
#include "keypad_view.h"
#include "main_view.h"
#include <memory>

namespace view_manager {
extern KeypadView keypadView;
extern KeyboardView keyboardView;
// extern NumberEditView numberEditView;
// extern OptionButtonsView optionButtonsView;
extern MainView mainView;
extern View *currentView;
extern os::periodic_task task;
void push(View *);
void pop();
void init();

void open_app(std::unique_ptr<View> view);
void open(std::unique_ptr<View> v);

bool on_input(st_inputEvent &e);

/**
 * Changes the focus from the currently focused widget descendant from top_widget to the nearest widget in the specified direction.
 * The eligible focusable widget can be restricted to be under a maximum of max_levels parent (if 0, all descendants of top_widget are considered)
 */
bool change_focus(Widget *const top_widget, ui::DIRECTION direction, uint32_t max_levels = 1);

} // namespace view_manager

#endif // VIEW_MANAGER_H
