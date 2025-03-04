/*
 * Filename: frequency_memory_ui.cpp
 * Author: Angel Dust
 * Date: 2025-02-22
 */

#include "frequency_memory_ui.h"
#include "menuBase.h"
#include "types.h"

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

int get_index();
void open_save_current();

// st_freq_mem temporary register
st_freq_mem tempFreqMem;
char tempFreqBuf[] = "000,000,000";
int curr_index = -1;
using namespace Menu;
// A function to save the edited data record
void saveTarget() {

    char *ptr;
    removePunct(tempFreqBuf);
    tempFreqMem.freq = strtol(tempFreqBuf, &ptr, 10);
    config.freqs[curr_index] = tempFreqMem;

    using namespace status;
    if (settings_write(&config) == HAL_FLASH_ERROR_NONE) {
        handleError(ST_INFO, "Configuration saved");
    } else {
        handleError(ST_ERROR, "Error saving configuration");
    }
}

void open_save_current() {

    view_manager::keyboardView.set_text("");
    view_manager::keyboardView.set_label("Name");
    view_manager::keyboardView.set_size(FREQ_MEM_NAME_SIZE);
    view_manager::keyboardView.on_changed = [](char *str) {
        curr_index = get_index();
        strncpy(tempFreqMem.name, str, FREQ_MEM_NAME_SIZE);
        tempFreqMem.freq = radio::get_frequency();
        tempFreqMem.mode = config.modulation;
        config.freqs[curr_index] = tempFreqMem;
        using namespace status;
        if (settings_write(&config) == HAL_FLASH_ERROR_NONE) {
            handleError(ST_INFO, "Saved");
        } else {
            handleError(ST_ERROR, "Error saving");
        }
    };
    view_manager::push((View *)&view_manager::keyboardView);
}

result edit_freq_name(eventMask, navNode &) {

    view_manager::keyboardView.set_text(tempFreqMem.name);
    view_manager::keyboardView.set_label("Name");
    view_manager::keyboardView.set_size(FREQ_MEM_NAME_SIZE);
    view_manager::keyboardView.on_changed = [](char *str) {
        strncpy(tempFreqMem.name, str, FREQ_MEM_NAME_SIZE);
        saveTarget();
    };
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
            saveTarget();
        },
        config.f_min, config.f_max);

    return proceed;
}

labelPrompt freqNameMenu((const char *)"Name", tempFreqMem.name, edit_freq_name, enterEvent, noStyle);
labelPrompt freqEditMenu((const char *)"Frequency", tempFreqBuf, edit_freq, enterEvent, noStyle);

optionsPrompt<MODULATION_MODE> modulationModeMenu((const char *)"Modulation", modulation_options, config.modulation,
                                                  sizeof(modulation_options) / sizeof(modulation_options[0]), [](MODULATION_MODE) { saveTarget(); });

MENU(freqMemEditMenu, "Frequency edit", doNothing, noEvent, wrapStyle, OBJ(freqNameMenu), OBJ(modulationModeMenu), OBJ(freqEditMenu));

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
        curr_index = nav.sel;

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
