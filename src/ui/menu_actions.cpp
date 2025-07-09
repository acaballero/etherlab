/*
 * Filename: menu_actions.cpp
 * Author: Angel Dust
 * Date: 2025-03-13
 */

#include "menu_actions.h"
#include "Display_afb.h"
#include "menuBase.h"
#include "ui/view_manager.h"

namespace Menu {

menu_action_st navigation_actions_arr[] = {{"<-",
                                            []() {
                                                nav.doNav(downCmd);
                                                view_manager::mainView.Menu()->set_dirty();
                                            }},
                                           {"->",
                                            []() {
                                                nav.doNav(upCmd);
                                                view_manager::mainView.Menu()->set_dirty();
                                            }},
                                           {"x",
                                            []() {
                                                nav.doNav(enterCmd);
                                                view_manager::mainView.Menu()->set_dirty();
                                            },
                                            C565_GREEN_DARK},
                                           {"Back", []() {
                                                nav.doNav(escCmd);
                                                view_manager::mainView.Menu()->set_dirty();
                                            }}};
menu_actions_st navigation_actions = {navigation_actions_arr, 4};

} // namespace Menu
