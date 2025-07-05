/*
 * Filename: menu_actions.cpp
 * Author: Angel Dust
 * Date: 2025-03-13
 */

#include "menu_actions.h"
#include "menuBase.h"
#include "ui/view_manager.h"

namespace Menu {

menu_action_st navigation_actions_arr[] = {{"<-",
                                            []() {
                                                nav.doNav(downCmd);
                                                view_manager::mainView.Menu()->set_dirty();
                                            }},
                                           {"->", []() {
                                                nav.doNav(upCmd);
                                                view_manager::mainView.Menu()->set_dirty();
                                            }}};
menu_actions_st navigation_actions = {navigation_actions_arr, 2};

} // namespace Menu
