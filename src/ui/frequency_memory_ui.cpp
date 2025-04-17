/*
 * Filename: frequency_memory_ui.cpp
 * Author: Angel Dust
 * Date: 2025-02-22
 */

#include "frequency_memory_ui.h"
#include "config.h"
#include "itemsTemplates.hpp"
#include "main_board.h"
#include "menuBase.h"
#include "radio.h"
#include "types.h"
#include "ui/menu_actions.h"
#include "ui/menu_options.h"
#include "ui/ui_types.h"
#include <cstddef>

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
char tempFreqBuf[] = "00 000 000 000";
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

/**
 * Retrieves the index of a stored frequency by frequency and mode
 */
int find_index(st_freq_mem data) {

    uint16_t i = 0;
    for (; i < FREQ_MEM_SIZE; i++) {
        if (config.freqs[i].freq == data.freq && config.freqs[i].mode == data.mode) {
            return i;
        }
    }

    return -1;
}

/**
 * Retrieves a register by channel id
 */
st_freq_mem *find_id(uint16_t group, uint16_t id) {

    uint16_t i = 0;
    for (; i < FREQ_MEM_SIZE; i++) {
        if (config.freqs[i].id == id && config.freqs[i].group == group) {
            return &config.freqs[i];
        }
    }

    return nullptr;
}

void save_freq(st_freq_mem item, int i) {

    if (i < 0) { // Find by frequency
        i = find_index(item);
    }

    if (i < 0) {
        i = get_index();
    }

    item.id = i;

    using namespace status;

    if (i < 0) {
        handleError(ST_ERROR, "Memory full");
    } else {
        config.freqs[i] = item;
        using namespace status;
        if (settings_write(&config) == HAL_FLASH_ERROR_NONE) {
            handleError(ST_INFO, "Saved");
        } else {
            handleError(ST_ERROR, "Error saving");
        }
    }
}

void open_save_current() {

    view_manager::keyboardView.set_text("");
    view_manager::keyboardView.set_label("Name");
    view_manager::keyboardView.set_size(FREQ_MEM_NAME_SIZE);
    view_manager::keyboardView.on_changed = [](char *str) {
        strncpy(tempFreqMem.name, str, FREQ_MEM_NAME_SIZE);
        tempFreqMem.mode = config.modulation;
        tempFreqMem.freq = radio::get_frequency();
        save_freq(tempFreqMem);
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

void del_freq(int i) {
    config.freqs[i] = {};

    using namespace status;
    if (settings_write(&config) == HAL_FLASH_ERROR_NONE) {
        handleError(ST_INFO, "Deleted");
    } else {
        handleError(ST_ERROR, "Error deleting");
    }
}

st_freq_mem *next_prev(bool next) {

    int16_t step = next ? 1 : -1;
    uint16_t from_ix = curr_index >= 0 ? constrain(curr_index + step, 0, FREQ_MEM_SIZE) : 0;
    int16_t to_ix = next ? FREQ_MEM_SIZE : -1;

    uint16_t ix = from_ix;
    while (ix != to_ix) {

        if (config.freqs[ix].freq) {
            return &config.freqs[ix];
        }

        ix += step;
    }

    return nullptr;
}

void set(st_freq_mem *mem) {
    int ix = find_index(*mem);

    if (ix >= 0) {
        curr_index = ix;
    }

    main_board::setModulationMode(mem->mode, false);
    radio::set_frequency(mem->freq);
}

void set_next_prev(DIRECTION d) {
    if (get_memory_mode() && curr_index >= 0) {
        st_freq_mem *mem = find_closest(config.freqs[curr_index].freq, 0, d);
        if (mem) {
            freq_memory::set(mem);
        }
    } else {
    }
}

st_freq_mem *find_closest(uint64_t f, uint16_t group, DIRECTION direction = STOP) {
    int ix = -1;
    uint32_t min_distance = (uint32_t)-1;
    int32_t distance = 0;

    for (int i = 0; i < FREQ_MEM_SIZE; i++) {
        distance = config.freqs[i].freq - f;

        if (!config.freqs[i].freq || (direction == FORWARD && distance <= 0) || (direction == BACKWARDS && distance >= 0)) {
            continue;
        }

        distance = abs(distance);

        if ((uint32_t)distance < min_distance && config.freqs[i].group == group) {
            min_distance = distance;
            ix = i;
        }
    }

    if (ix >= 0) {
        return &config.freqs[ix];

    } else {
        return nullptr;
    }
}

bool get_memory_mode() {
    return config.memory_mode;
}

uint8_t toggle_memory_mode() {
    bool memory_mode = (config.memory_mode == 0 ? 1 : 0);

    if (memory_mode) {
        // Are there any frequencies
        st_freq_mem *mem = find_closest(radio::get_frequency(), 0);

        if (mem) {
            freq_memory::set(mem);
        } else {
            using namespace status;
            handleError(ST_ERROR, "Frequency memory empty");
            return 1;
        }
    }

    config.memory_mode = memory_mode;

    return 0;
}

labelPrompt freqNameMenu((const char *)"Name", tempFreqMem.name, edit_freq_name, enterEvent, noStyle);
labelPrompt freqEditMenu((const char *)"Frequency", tempFreqBuf, edit_freq, enterEvent, noStyle);

optionsPrompt<MODULATION_MODE> modulationModeMenu((const char *)"Modulation", modulation_options, config.modulation,
                                                  sizeof(modulation_options) / sizeof(modulation_options[0]), [](MODULATION_MODE) {
                                                      saveTarget();
                                                  });

MENU(freqMemEditMenu, "Frequency edit", doNothing, noEvent, wrapStyle, OBJ(freqNameMenu), OBJ(modulationModeMenu), OBJ(freqEditMenu));

result freqMemorySelectedEvent(eventMask e, navNode &nav);

FreqMemoryMenu freqMemMenu("Frequency memory", FREQ_MEM_SIZE, nullptr, freqMemEditMenu, freqMemorySelectedEvent, (eventMask)(enterEvent | exitEvent));

menu_action_st menu_actions[] = {navigation_actions_arr[Menu::UP], navigation_actions_arr[Menu::DOWN], {"Delete", []() {
                                                                                                            if (freqMemMenu.curr_ix >= 0) {
                                                                                                                del_freq(freqMemMenu.curr_ix);
                                                                                                            }
                                                                                                        }}};

menu_actions_st actions = {menu_actions, sizeof(menu_actions) / sizeof(menu_action_st)};

/*
 * This will be called whenever an entry is selected in the frequency memory
 * It copies the currently selected index st_freq_mem in the temporary struct
 */
result freqMemorySelectedEvent(eventMask e, navNode &nav) {
    // trace(MENU_DEBUG_OUT << "copy data to temp target:" << (int)nav.target << "\n");
    if (nav.target == &freqMemMenu && freqMemMenu.curr_ix >= 0) { // Only if we are on memory menu
        tempFreqMem = config.freqs[freqMemMenu.curr_ix];

        // If it's empty: New entry. Use current frequency
        if (!tempFreqMem.freq) {
            tempFreqMem.freq = radio::get_frequency();
            tempFreqMem.mode = config.modulation;
            curr_index = -1;
        } else {
            curr_index = nav.sel;
        }

        char buf[16];
        format_long(tempFreqMem.freq, buf);
        sprintf(tempFreqBuf, "%s", buf);
    }

    if (e == Menu::enterEvent) {

        if (!actions.actions[0].action) {
            for (size_t i = 0; i < navigation_actions.size; i++) {
                actions.actions[i] = navigation_actions.actions[i];
            }
        }

        actions_signal.emit(&actions);
    } else if (e == Menu::exitEvent) {
        // Remove context actions
        actions_signal.emit(nullptr);
    }

    // nav.sel can be stored for future reference
    return proceed;
}

} // namespace freq_memory
