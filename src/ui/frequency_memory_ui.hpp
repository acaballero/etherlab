//
// Created by Angel Dust on 03/02/2025.
//
#ifndef TRX_FRONTEND_FREQUENCY_MEMORY_UI_H
#define TRX_FRONTEND_FREQUENCY_MEMORY_UI_H

#include "types.h"
#include "config.h"
#include "settings.h"
#include "status.h"
#include "view_manager.h"
#include "keyboard_view.h"
#include "../lib/Menu/src/plugin/userMenu.h"
#include "menu_options.h"
#include "menu_prompts.h"

using namespace Menu;

// st_freq_mem temporary register
st_freq_mem tempFreqMem;
char tempFreqBuf[] = "000,000,000";

// Character validators for the frequency memories
const char *constMEM alphaNum MEMMODE = " 0123456789.ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,\\|!\"#$%&/()=?~*^+-{}[]€";
const char *constMEM alphaNumMask[1] MEMMODE = {alphaNum};
const char *constMEM digit MEMMODE = "0123456789";
const char *constMEM digitMask[] MEMMODE = {digit, digit, digit, ","};

// A function to save the edited data record
result saveTarget(eventMask, navNode &nav) {
    trace(MENU_DEBUG_OUT << "saveTarget" << endl);
    navNode &nn = nav.root->path[nav.root->level - 1];
    idx_t n = nn.sel; // get selection of previous level
    char *ptr;
    removePunct(tempFreqBuf);
    tempFreqMem.freq = strtol(tempFreqBuf, &ptr, 10);
    config.freqs[n] = tempFreqMem;

    using namespace status;
    if (settings_write(&config) == HAL_FLASH_ERROR_NONE) {
        handleError(ST_INFO, "Configuration saved");
    } else {
        handleError(ST_ERROR, "Error saving configuration");
    }
    return quit;
}

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

result edit_freq_name(eventMask, navNode &) {

    view_manager::keyboardView.set_text(tempFreqMem.name);
    view_manager::keyboardView.set_label("Name");
    view_manager::keyboardView.set_size(FREQ_MEM_NAME_SIZE);
    view_manager::keyboardView.on_changed = [](char *str) { strncpy(tempFreqMem.name, str, FREQ_MEM_NAME_SIZE); };
    view_manager::push(&view_manager::keyboardView);
    return proceed;
}

result edit_freq(eventMask, navNode &) {
    Menu::open_keypad<uint64_t>(
        tempFreqMem.freq, "Hz", "Frequency", 0, false,
        [](uint64_t v) {
            tempFreqMem.freq = v;
            char buf[16];
            format_long(tempFreqMem.freq, buf);
            sprintf(tempFreqBuf, "%s", buf);
        },
        config.f_min, config.f_max);

    return proceed;
}

labelPrompt freqNameMenu((const char *)"Name", tempFreqMem.name, edit_freq_name, enterEvent, noStyle);
labelPrompt freqEditMenu((const char *)"Frequency", tempFreqBuf, edit_freq, enterEvent, noStyle);

optionsPrompt<MODULATION_MODE> modulationModeMenu((const char *)"Modulation", modulation_options, config.modulation,
                                                  sizeof(modulation_options) / sizeof(modulation_options[0]), nullptr);

MENU(freqMemEditMenu, "Frequency edit", doNothing, noEvent, wrapStyle, OBJ(freqNameMenu), OBJ(modulationModeMenu), OBJ(freqEditMenu),
     OP("Save", saveTarget, enterEvent));

// Custom frequency memory menu
struct FreqMemoryMenu : UserMenu {
    using UserMenu::UserMenu;

    // Override sz() function to have variable/custom size
    // If using exit option an extra element has to be considered...
    // inline idx_t sz() const override {return 0;}

    Used printItem(menuOut &out, int idx, int len) override {

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
            case idxCmd: // Index selected

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

result freqMemorySelectedEvent(eventMask e, navNode &nav);

FreqMemoryMenu freqMemMenu("Frequency memory", FREQ_MEM_SIZE, "<Back", freqMemEditMenu, freqMemorySelectedEvent, enterEvent);

/*
 * This will be called whenever an entry is selected in the frequency memory
 * It copies the currently selected index st_freq_mem in the temporary struct
 */
result freqMemorySelectedEvent(eventMask e, navNode &nav) {
    trace(MENU_DEBUG_OUT << "copy data to temp target:" << (int)nav.target << "\n");
    if (nav.target == &freqMemMenu) { // Only if we are on memory menu
        tempFreqMem = config.freqs[nav.sel];

        // If it's empty: New entry. Use current frequency
        if (!tempFreqMem.freq) {
            tempFreqMem.freq = radio::get_frequency();
            tempFreqMem.mode = config.modulation;
        }

        char buf[16];
        format_long(tempFreqMem.freq, buf);
        sprintf(tempFreqBuf, "%s", buf);
    }
    // nav.sel can be stored for future reference
    return proceed;
}

#endif
