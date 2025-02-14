//
// Created by Angel Dust on 03/02/2025.
//

#ifndef TRX_FRONTEND_MENU_PROMPTS_H
#define TRX_FRONTEND_MENU_PROMPTS_H

#include "../../lib/Menu/src/menu.h"
#include "menuBase.h"
#include "menu_options.h"
#include "ui/menuILI9431Out.h"
#include "view_manager.h"
#include <cstdint>
#include <functional>

namespace Menu {

template <typename T = double>
void open_keypad(T value, const char *units, const char *name, uint8_t frac_digits, bool with_multipliers, std::function<void(T)> on_changed, T min, T max);

template <typename T>
void open_number_edit(T value, const char *units, const char *name, uint8_t frac_digits, std::function<void(T)> on_changed, T min, T max, T step, T step_big) {

    view_manager::mainView.NumberEdit()->set_value(value, frac_digits, units, name, min, max, step, step_big);
    view_manager::mainView.NumberEdit()->on_changed = on_changed;
    view_manager::mainView.NumberEdit()->set_visible(true);
}

template <typename T> void open_option_buttons(menu_options_t<T> options, const char *title, T &value, uint16_t size, std::function<void(T)> on_select) {

    view_manager::mainView.OptionButtons()->clear();

    const auto fn = [&value, options, on_select](uint16_t index) {
        view_manager::mainView.OptionButtons()->set_visible(false);

        value = options[index].value;
        if (on_select) {
            on_select(value);
        }
    };

    for (int i = 0; i < size; i++) {
        menu_option_st<T> option = options[i];

        view_manager::mainView.OptionButtons()->on_select = fn;
        view_manager::mainView.OptionButtons()->add_item(option.name, nullptr, value == option.value, option.fg_color, option.bg_color);
    }

    view_manager::mainView.OptionButtons()->set_title(title);
    view_manager::mainView.OptionButtons()->set_visible(true);
}

template <typename T>
void open_keypad(T value, const char *units, const char *name, uint8_t frac_digits, bool with_multipliers, std::function<void(T)> on_changed, T min, T max) {

    view_manager::keypadView.set_value(value, frac_digits, units, name, min, max);
    view_manager::keypadView.with_multipliers(with_multipliers);
    view_manager::keypadView.on_changed = on_changed;
    view_manager::push(&view_manager::keypadView);
}

class labelPrompt : public Menu::prompt {
  public:
    char *value;

    labelPrompt(const char *text, char *value, action a = doNothing, eventMask e = noEvent, styles s = noStyle,
                systemStyles ss = ((Menu::systemStyles)(Menu::_parentDraw)))
        : prompt(text, a, e, s, ss), value(value) {}
    Used printTo(navRoot &, bool sel, menuOut &out, idx_t, idx_t len, idx_t) override {
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
                  styles s = noStyle, systemStyles ss = ((Menu::systemStyles)(Menu::_parentDraw)))
        : prompt(text, static_cast<action>([](Menu::eventMask, Menu::navNode &, Menu::prompt &item) {
                     optionsPrompt<T> prompt = static_cast<optionsPrompt<T> &>(item);
                     open_option_buttons<T>(prompt.options, item.getText(), prompt.value, prompt.size, [prompt](T m) {
                         if (prompt.on_select) {
                             prompt.on_select(m);
                         }
                     });
                     return proceed;
                 }),
                 e, s, ss),
          value(value), options(options), size(size), on_select(on_select) {}

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
        size_t i;
        for (i = 0; i < size && options[i].value != value; i++) {
            ;
        }
        return i >= size ? &options[0] : &options[i]; // Defaults to first element if not found
    }
};

template <typename T> class numberPrompt : public Menu::prompt {
  public:
    T *value;
    uint8_t decimals;
    char thow_separator;
    char dec_separator;
    const char *unit = nullptr;
    T min;
    T max;
    T step;
    T step_big;
    std::function<void(T)> on_select;

    numberPrompt(const char *text, T *value, uint8_t decimals = 0, char thow_separator = ' ', char dec_separator = '.', const char *unit = nullptr,
                 std::function<void(T)> on_select = nullptr, T min = T{}, T max = T{}, T step = T{}, T step_big = T{}, eventMask e = enterEvent,
                 styles s = noStyle, systemStyles ss = ((Menu::systemStyles)(Menu::_parentDraw)))
        : prompt(text, static_cast<action>([](Menu::eventMask e, Menu::navNode &, Menu::prompt &item) {
                     numberPrompt<T> prompt = static_cast<numberPrompt<T> &>(item);

                     if (e == Menu::enterEvent) {
                         if (prompt.step > 0) {
                             Menu::open_number_edit<T>(
                                 *prompt.value, prompt.unit, prompt.shadow->text, prompt.decimals,
                                 [prompt](T v) {
                                     *prompt.value = v;
                                     if (prompt.on_select) {
                                         prompt.on_select(v);
                                     }
                                 },
                                 prompt.min, prompt.max, prompt.step, prompt.step_big);
                         } else {
                             Menu::open_keypad<T>(
                                 *prompt.value, prompt.unit, prompt.shadow->text, prompt.decimals, false,
                                 [prompt](T v) {
                                     *prompt.value = v;
                                     if (prompt.on_select) {
                                         prompt.on_select(v);
                                     }
                                 },
                                 prompt.min, prompt.max);
                         }
                     }

                     return proceed;
                 }),
                 e, s, ss),
          value(value), decimals(decimals), thow_separator(thow_separator), dec_separator(dec_separator), unit(unit), min(min), max(max), step(step),
          step_big(step_big), on_select(on_select) {}

    idx_t printTo(navRoot &, bool sel, menuOut &out, idx_t, idx_t len, idx_t) override;
};

template class numberPrompt<uint16_t>;
template class numberPrompt<uint32_t>;
template class numberPrompt<uint64_t>;
template class numberPrompt<int16_t>;
template class numberPrompt<int>;

} // namespace Menu

#endif
