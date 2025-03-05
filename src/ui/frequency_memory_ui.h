//
// Created by Angel Dust on 03/02/2025.
//
#ifndef TRX_FRONTEND_FREQUENCY_MEMORY_UI_H
#define TRX_FRONTEND_FREQUENCY_MEMORY_UI_H

#include "types.h"
#include "config.h"
#include "menu.h"
#include "settings.h"
#include "status.h"
#include "view_manager.h"
#include "keyboard_view.h"
#include "../lib/Menu/src/plugin/userMenu.h"
#include "menu_options.h"
#include "menu_prompts.h"
#include <sys/_stdint.h>

namespace freq_memory {

int get_index();
int find_index(st_freq_mem);
void open_save_current();
void save_freq(st_freq_mem item);

// Custom frequency memory menu
struct FreqMemoryMenu : Menu::UserMenu {
    using UserMenu::UserMenu;

    // Override sz() function to have variable/custom size
    // If using exit option an extra element has to be considered...
    // inline idx_t sz() const override {return 0;}

    Menu::Used printItem(Menu::menuOut &out, int idx, int len) override {

        if (len) {
            char buf[35], sf[10];
            bool empty;
            st_freq_mem fm = config.freqs[idx];
            empty = fm.freq == 0;
            if (empty) {
                sprintf(buf, "[%2d]", idx);
            } else {
                format_long(fm.freq, sf);
                snprintf(buf, 35, "[%2d] %-4s %12s  %*s", idx, radio::modulationNames[fm.mode], sf, FREQ_MEM_NAME_SIZE, fm.name);
            }

            return out.printText(buf, 35);
        } else {
            return 0;
        }
    }

    void doNav(Menu::navNode &nav, Menu::navCmd cmd) override {

        switch (cmd.cmd) {
            case Menu::idxCmd: // Index selected

                if (config.freqs[nav.sel].freq) {
                    radio::set_frequency(config.freqs[nav.sel].freq);
                }

                // Exit menu
                // UserMenu::doNav(nav, Menu::escCmd);
                break;

            default:
                UserMenu::doNav(nav, cmd);
                break;
        }
    }
};

extern FreqMemoryMenu freqMemMenu;
} // namespace freq_memory

namespace Menu {

template <typename T> idx_t numberPrompt<T>::printTo(navRoot &, bool sel, menuOut &out, idx_t, idx_t len, idx_t) {
    len -= out.printRaw(shadow->text, len);
    len -= out.printRaw(": ", len);
    out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
    char buf[20];

    if (std::is_same<T, double>::value || std::is_same<T, float>::value) {
        ftoa(buf, 20, *value, decimals);
    } else {
        format_long((int64_t)*value, buf, 0, thow_separator);
    }

    len -= out.printRaw(buf, len);

    out.setColor(Menu::unitColor, sel, Menu::enabledStatus, false);
    len -= out.printRaw(" ", len);
    len -= out.printRaw(unit, len);
    return len;
}
} // namespace Menu

#endif
