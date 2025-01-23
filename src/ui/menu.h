#ifndef __MENU_H
#define __MENU_H

#include "hw/stm32.h"
#include <math.h>
#include <sys/_stdint.h>
#include <type_traits>
#include "../../lib/Menu/src/menu.h"
#include "menuBase.h"
#include "utils.hpp"

enum MenuStatus { ACTIVE, IDLE, UNKNOWN };
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

namespace Menu {
class labelPrompt : public Menu::prompt {
  public:
    char *value;

    labelPrompt(const char *text, char *value, action a = doNothing, eventMask e = noEvent, styles s = noStyle,
                systemStyles ss = ((Menu::systemStyles)(Menu::_parentDraw)))
        : prompt(text, a, e, s, ss), value(value) {}
    Used printTo(navRoot &root, bool sel, menuOut &out, idx_t idx, idx_t len, idx_t) override {
        len -= out.printRaw(shadow->text, len);
        len -= out.printRaw(": ", len);
        out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
        len -= out.printRaw(value, len);

        return len;
    }
};

template <typename T> class numberPrompt : public Menu::prompt {
  public:
    T *value;
    double min;
    double max;
    uint8_t decimals;
    char thow_separator;
    char dec_separator;
    const char *unit = "Hz";

    numberPrompt(const char *text, T *value, uint8_t decimals = 0, char thow_separator = ' ', char dec_separator = '.', action a = doNothing,
                 eventMask e = enterEvent, styles s = noStyle, systemStyles ss = ((Menu::systemStyles)(Menu::_parentDraw)))
        : prompt(text, a, e, s, ss), value(value), decimals(decimals), thow_separator(thow_separator), dec_separator(dec_separator) {}

    idx_t printTo(navRoot &, bool sel, menuOut &out, idx_t, idx_t len, idx_t) override;

    result eventHandler(eventMask e, navNode &, idx_t) override;
};

} // namespace Menu

#endif
