/*
 * Filename: frequency_memory_ui.cpp
 * Author: Angel Dust
 * Date: 2025-02-22
 */

#include "frequency_memory_ui.h"
#include "menuBase.h"

namespace freq_memory {

int get_index() {
    int i = 0;
    while (i < FREQ_MEM_SIZE) {
        if (!config.freqs[i].freq) {
            return i;
        }
        i++;
    }

    return -1;
}
void open_save_current() {

    Menu::idx_t index = get_index();
    if (index >= 0) {
        nav.doNav(Menu::navCmd(Menu::enterCmd));
        nav.doNav(Menu::navCmd(Menu::idxCmd, 6));
        for (int i = 0; i < index; i++) {
            nav.doNav(Menu::navCmd(Menu::upCmd));
        }
        nav.doNav(Menu::navCmd(Menu::enterCmd));
        nav.doNav(Menu::navCmd(Menu::enterCmd));
    }
}
} // namespace freq_memory

namespace freq_memory {

int get_index();
void open_save_current();

// st_freq_mem temporary register
st_freq_mem tempFreqMem;
char tempFreqBuf[] = "000,000,000";

using namespace Menu;
// A function to save the edited data record
Menu::result saveTarget(eventMask, navNode &nav) {
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

result edit_freq_name(eventMask, navNode &) {

    view_manager::keyboardView.set_text(tempFreqMem.name);
    view_manager::keyboardView.set_label("Name");
    view_manager::keyboardView.set_size(FREQ_MEM_NAME_SIZE);
    view_manager::keyboardView.on_changed = [](char *str) { strncpy(tempFreqMem.name, str, FREQ_MEM_NAME_SIZE); };
    view_manager::push((View *)&view_manager::keyboardView);
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

result freqMemorySelectedEvent(eventMask e, navNode &nav);

FreqMemoryMenu freqMemMenu("Frequency memory", FREQ_MEM_SIZE, "<Back", freqMemEditMenu, freqMemorySelectedEvent, enterEvent);

/*
 * This will be called whenever an entry is selected in the frequency memory
 * It copies the currently selected index st_freq_mem in the temporary struct
 */
result freqMemorySelectedEvent(eventMask e, navNode &nav) {
    // trace(MENU_DEBUG_OUT << "copy data to temp target:" << (int)nav.target << "\n");
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

} // namespace freq_memory
