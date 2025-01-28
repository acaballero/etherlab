#ifndef __MENU_H
#define __MENU_H

#include "Display_afb.h"
#include "hw/stm32.h"
#include <functional>
#include <math.h>
#include <sys/_stdint.h>
#include <type_traits>
#include "../../lib/Menu/src/menu.h"
#include "menuBase.h"
#include "ui/menuILI9431Out.h"
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

namespace Menu {

template <typename T> struct menu_option_st {
    const char *name;
    T value;
    uint16_t fg_color = C565_TEXT_FG;
    uint16_t bg_color = C565_TEXT_BG;
};

template <typename T> using menu_options_t = menu_option_st<T> *;

extern menu_option_st<uint16_t> color_options[23];

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

template <typename T> class optionsPrompt : public Menu::prompt {
  public:
    T &value;
    menu_options_t<T> options;
    size_t size;
    std::function<void(T)> on_select;

    optionsPrompt(const char *text, menu_options_t<T> options, T &value, size_t size, std::function<void(T)> on_select = nullptr, eventMask e = enterEvent,
                  styles s = noStyle, systemStyles ss = ((Menu::systemStyles)(Menu::_parentDraw)));

    Used printTo(navRoot &, bool sel, menuOut &out, idx_t, idx_t len, idx_t) override {

        menu_option_st<T> *option = find_option(value);
        len -= out.printRaw(shadow->text, len);
        len -= out.printRaw(": ", len);
        out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
        reinterpret_cast<menuILI9431Out *>(&out)->gfx.setBgColor(option->bg_color);
        len -= out.printRaw(option->name, len);
        out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);

        return len;
    }

  protected:
    menu_option_st<T> *find_option(T &value) {
        int i;
        for (i = 0; i < size && options[i].value != value; i++) {
            ;
        }
        return i >= size ? &options[0] : &options[i]; // Defaults to first element if not found
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
