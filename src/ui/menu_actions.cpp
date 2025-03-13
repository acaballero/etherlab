/*
 * Filename: menu_actions.cpp
 * Author: Angel Dust
 * Date: 2025-03-13
 */

#include "menu_actions.h"
#include "menuBase.h"

namespace Menu {

menu_action_st navigation_actions_arr[] = {{"<-", []() { nav.doNav(downCmd); }}, {"->", []() { nav.doNav(upCmd); }}};
menu_actions_st navigation_actions = {navigation_actions_arr, 2};

} // namespace Menu
