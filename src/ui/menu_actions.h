/*
 * Filename: menu_actions.h
 * Author: Angel Dust
 * Date: 2025-03-13
 */

#ifndef MENU_ACTIONS_H
#define MENU_ACTIONS_H

#include "menu_options.h"

namespace Menu {

enum NAVIGATION_ACTIONS { UP = 0, DOWN };
menu_actions_st &get_navigation_actions();

} // namespace Menu

#endif //  MENU_ACTIONS_H
