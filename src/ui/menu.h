#ifndef __MENU_H
#define __MENU_H

#include "Display_afb.h"
#include "hw/stm32.h"
#include <functional>
#include <math.h>

#include <type_traits>
#include "../../lib/Menu/src/menu.h"
#include "menuBase.h"
#include "ui/menuILI9431Out.h"
#include "utils.hpp"
#include "menu_options.h"
#include "menu_prompts.h"

Menu::result doAlert(Menu::eventMask e, Menu::prompt &item);
void menu_setup();
void menu_exit();

Menu::result updateRadio(Menu::eventMask);
void menu_size(int w, int h);
Menu::result changeAutoSquelch(Menu::eventMask e);
Menu::result changeAGCEnabled(Menu::eventMask e);
extern Menu::navRoot nav;
extern Menu::menuNode fileSubmenu;

extern const char *constMEM alphaNum MEMMODE;
extern const char *constMEM alphaNumMask[1] MEMMODE;

namespace Menu {
extern Menu::MenuStatus menuStatus;

// Character validators for the frequency memories
extern const char *constMEM alphaNum MEMMODE;
extern const char *constMEM alphaNumMask[1] MEMMODE;
extern const char *constMEM digit MEMMODE;
extern optionsPrompt<radio::FRONTEND_PATH> frontendPathMenu;
extern optionsPrompt<radio::RPT_MODE> repeaterMenu;
extern optionsPrompt<MODULATION_MODE> modulationMenu;
extern optionsPrompt<radio::BAND> bandMenu;
extern optionsPrompt<radio::BAND> filterMenu;
extern optionsPrompt<radio::IF_FILTER> IFFilterMenu;
extern Menu::numberPrompt<float> squelchEditMenu;
void open_gain();
} // namespace Menu

#endif
