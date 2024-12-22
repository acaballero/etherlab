#ifndef __MENU_H
#define __MENU_H

#include "hw/stm32.h"
#include <math.h>
#include "../../lib/Menu/src/menu.h"

enum MenuStatus { ACTIVE, IDLE };
Menu::result doAlert(Menu::eventMask e, Menu::prompt &item);
void menu_setup();
void menu_exit();
Menu::result updateRadio(Menu::eventMask);
void menu_size(int w, int h);
Menu::result changeAutoSquelch(Menu::eventMask e);
Menu::result changeAGCEnabled(Menu::eventMask e);
extern Menu::navRoot nav;
extern Menu::menuNode fileSubmenu;
extern enum MenuStatus menuStatus;
extern const char *constMEM alphaNum MEMMODE;
extern const char *constMEM alphaNumMask[1] MEMMODE;
extern Menu::prompt *colorValues[23];
#endif
