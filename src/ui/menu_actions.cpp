/*
 * Filename: menu_actions.cpp
 * Author: Angel Dust
 * Date: 2025-03-13
 */

#include "menu_actions.h"
#include "Display_afb.h"
#include "input/inputEvent.h"
#include "menuBase.h"
#include "ui/button_widget.h"
#include "ui/view_manager.h"
#include "input/input_controller.h"

namespace Menu {

menu_action_st navigation_actions_arr[] = {{"<-",
                                            []() {
                                                input_controller::queue_input_event({INPUT_EVENT_TYPE_ENCODER, -1});
                                            }},
                                           {"->",
                                            []() {
                                                input_controller::queue_input_event({INPUT_EVENT_TYPE_ENCODER, 1});
                                            }},
                                           {"x",
                                            []() {
                                                input_controller::queue_input_event({INPUT_EVENT_TYPE_BUTTON_PRESS, BTN_ENCODER, 0});
                                            },
                                            C565_GREEN_DARK},
                                           {"Back", []() {
                                                input_controller::queue_input_event({INPUT_EVENT_TYPE_BUTTON_PRESS, KEY_BACK, 0});
                                            }}};
menu_actions_st navigation_actions = {navigation_actions_arr, 4};

} // namespace Menu
