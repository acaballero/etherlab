//
// Created by Angel Dust on 03/02/2025.
// Updated to use FileBuffer instead of in-memory array
//
#ifndef TRX_FRONTEND_FREQUENCY_MEMORY_UI_H
#define TRX_FRONTEND_FREQUENCY_MEMORY_UI_H

#include "menuBase.h"
#include "types.h"
#include "config.h"
#include "menu.h"
#include "settings.h"
#include "status.h"
#include "view_manager.h"
#include "keyboard_view.h"
#include "../lib/Menu/src/plugin/userMenu.h"
#include "menu_options.h"
#include <sys/_stdint.h>

namespace freq_memory {

// FileBuffer functions
bool init_file_buffer();
st_freq_mem get_by_index(int index);
int get_freq_mem_count();

// Original interface functions (now using FileBuffer internally)
int find_index(st_freq_mem &);
void open_save_current();
void save(st_freq_mem &item);
void del_freq(int ix);
void set(st_freq_mem &);
st_freq_mem next_prev(bool next);
st_freq_mem find_closest(uint64_t frequency, DIRECTION);
void find_in_freq_range(uint64_t freq_min, uint64_t freq_max, std::vector<st_freq_mem> &out_memories);
void set_next_prev(DIRECTION d);
uint8_t toggle_memory_mode();
bool get_memory_mode();

// Custom frequency memory menu (updated for FileBuffer)
struct FreqMemoryMenu : Menu::UserMenu {
    using UserMenu::UserMenu;

    int curr_ix = -1;

    // Override sz() function to have variable/custom size
    // If using exit option an extra element has to be considered...
    // inline idx_t sz() const override {return 0;}

    Menu::Used printItem(Menu::menuOut &out, int idx, int len) override {

        if (len) {
            static constexpr int buf_size = FREQ_MEM_NAME_SIZE + 27;
            char buf[buf_size], sf[14];
            bool empty;

            // Get frequency memory from FileBuffer instead of config.freqs
            st_freq_mem fm = get_by_index(idx);
            empty = fm.freq == 0;

            if (empty) {
                sprintf(buf, "[%2d]", idx);
            } else {
                format_long(fm.freq, sf);
                snprintf(buf, buf_size, "[%2d] %-4s %14s  %-*s", idx, radio::modulation_names[fm.mode], sf, FREQ_MEM_NAME_SIZE, fm.name);
            }

            return out.printText(buf, buf_size);
        } else {
            return 0;
        }
    }

    void doNav(Menu::navNode &nav, Menu::navCmd cmd) override {

        switch (cmd.cmd) {
            case Menu::idxCmd:             // long clicked
                UserMenu::doNav(nav, cmd); // TODO: Make this the delete command
                break;

            case Menu::enterCmd: // clicked
            {
                st_freq_mem fm = get_by_index(nav.sel);
                if (fm.freq) {
                    set(fm);
                }
            } break;
            default:
                UserMenu::doNav(nav, cmd);
                break;
        }

        curr_ix = nav.sel;
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
