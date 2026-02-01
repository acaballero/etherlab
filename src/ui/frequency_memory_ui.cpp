/*
 * Filename: frequency_memory_ui.cpp
 * Author: Angel Dust
 * Date: 2025-02-22
 * Modified to use FileBuffer instead of in-memory array
 */

#include "frequency_memory_ui.h"
#include "config.h"
#include "fatfs/fatfs.h"
#include "io/file_wrapper.h"
#include "itemsTemplates.hpp"
#include "main_board.h"
#include "menuBase.h"
#include "radio.h"
#include "status.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "types.h"
#include "ui/menu_actions.h"
#include "ui/menu_options.h"
#include "ui/menu_widget.h"
#include "ui/ui_types.h"
#include "result.h"
#include <cstddef>
#include <sstream>

#include "ui/view_manager.h"
#include "utils.hpp"

namespace freq_memory {

#define MAX_RETRIEVED_ITEMS_PER_RANGE 10 // Use this to save memory
#define INIT_OR_ABORT(value)                                                                                                                                   \
    {                                                                                                                                                          \
        if (!init_file_buffer()) {                                                                                                                             \
            return value;                                                                                                                                      \
        }                                                                                                                                                      \
    }

// FileBuffer for frequency memory storage
static std::unique_ptr<io::FileWrapper<>> db_file = nullptr;
int curr_index = -1;
static const char *FREQ_MEMORY_FILE = "madrid.db";

// Serialize frequency memory entry to string
std::string serialize_freq_mem(const st_freq_mem &mem) {
    std::ostringstream oss;
    oss << mem.id << "," << static_cast<int>(mem.mode) << "," << static_cast<int>(mem.type) << "," << mem.width << "," << mem.freq << "," << mem.repeater << ","
        << mem.offset << "," << mem.name;
    return oss.str();
}

bool parse_int_bounded(const char *start, size_t len, int &result) {
    if (len == 0 || len > 10) {
        return false; // Basic validation
    }

    char temp[12];
    memcpy(temp, start, len);
    temp[len] = '\0';

    return parse_int(temp, result);
}

bool parse_long_bounded(const char *start, size_t len, int64_t &result) {
    if (len == 0 || len > 20) {
        return false; // Basic validation
    }

    char temp[24];
    memcpy(temp, start, len);
    temp[len] = '\0';

    return parse_long(temp, result);
}

st_freq_mem deserialize_freq_mem(const char *line) {
    st_freq_mem mem = {};

    const char *current = line;
    const char *field_start = line;
    int field = 0;
    int val;

    while (*current != '\0' && field < 8) {
        // Find field end
        while (*current != ',' && *current != '\0') {
            current++;
        }

        size_t field_len = current - field_start;

        // Parse field based on type - no temporary string creation
        switch (field) {
            case 0: // index
                if (!parse_int_bounded(field_start, field_len, val)) {
                    return {};
                }
                mem.id = val;
                break;

            case 1: // mode
            {
                int temp;
                if (!parse_int_bounded(field_start, field_len, temp)) {
                    return {};
                }
                mem.mode = static_cast<MODULATION_MODE>(temp);
            } break;

            case 2: // band type
            {
                int temp;
                if (!parse_int_bounded(field_start, field_len, temp)) {
                    return {};
                }
                mem.type = static_cast<FREQ_TYPE>(temp);
            } break;

            case 3: // width
                if (!parse_int_bounded(field_start, field_len, val)) {
                    return {};
                }
                mem.width = val;
                break;

            case 4: // frequency
            {
                int64_t freq_val;
                if (!parse_long_bounded(field_start, field_len, freq_val)) {
                    return {};
                }
                mem.freq = static_cast<uint64_t>(freq_val);
            } break;

            case 5: // repeater
                if (!parse_int_bounded(field_start, field_len, val)) {
                    return {};
                }
                mem.repeater = val;
                break;

            case 6: // offset
                if (!parse_int_bounded(field_start, field_len, val)) {
                    return {};
                }
                mem.offset = val;
                break;

            case 7: // name
            {
                size_t copy_len = (field_len < FREQ_MEM_NAME_SIZE - 1) ? field_len : FREQ_MEM_NAME_SIZE - 1;
                memcpy(mem.name, field_start, copy_len);
                mem.name[copy_len] = '\0';
            } break;
        }

        field++;
        if (*current == ',') {
            current++;
            field_start = current;
        } else {
            break;
        }
    }

    return mem;
}

std::vector<st_freq_mem> get_all() {

    std::vector<st_freq_mem> memories;
    memories.reserve(db_file->line_count());

    for (uint32_t i = 0; i < db_file->line_count(); ++i) {
        std::string line = db_file->get_line(i);
        if (!line.empty()) {
            memories.push_back(deserialize_freq_mem(line.c_str()));
        }
    }

    return memories;
}

// Build sorted database from unsorted data
void build_sorted_database(std::vector<st_freq_mem> &memories) {

    // Sort by frequency
    std::sort(memories.begin(), memories.end(), [](const st_freq_mem &a, const st_freq_mem &b) {
        return a.freq < b.freq;
    });

    db_file->backup();
    db_file->clear();

    for (size_t i = 0; i < memories.size(); ++i) {
        std::string serialized = serialize_freq_mem(memories[i]);
        if (i < db_file->line_count()) {
            db_file->replace_line(i, serialized);
        } else {
            db_file->append_line(serialized);
        }
    }
}

// void log_file() {
//     FatFSFile *f = db_file->get_file();
//     size_t pos = f->tell();
//     f->seek(0);

//     char copy_buffer[BUFFER_SIZE];

//     while (true) {
//         auto read_result = f->read(copy_buffer, BUFFER_SIZE);
//         if (read_result.is_error() || *read_result == 0) {
//             LOG("Empty!\n");
//             break;
//         }

//         if (*read_result < BUFFER_SIZE) {
//             copy_buffer[*read_result] = 0;
//         }
//         LOG("********\n%s*********\n", copy_buffer);

//         if (*read_result < BUFFER_SIZE) {
//             break;
//         }
//     }

//     f->seek(pos);
// }

// void test() {
//     INIT_OR_ABORT()
//     if (db_file->load("test.db", true)) {
//         LOG("loaded test.db\n");
//         log_file();
//         db_file->clear();
//         LOG("Truncated\n");
//         log_file();
//         int n = 3;
//         std::string name;
//         for (int i = 0; i < n; i++) {
//             name = (std::string("NAME_") + std::to_string(i));
//             LOG("Inserting %s\n", name.c_str());
//             st_freq_mem m{0, (uint16_t)i, (uint64_t)100 * (i + 1), FM, name.c_str()};
//             save(m);
//             log_file();
//             db_file->log();
//         }
//         name = (std::string("NEW_BIGGER_1"));
//         LOG("Inserting %s\n", name.c_str());
//         st_freq_mem m1{0, (uint16_t)(n + 1), (uint64_t)50, FM, name.c_str()};
//         save(m1);
//         log_file();
//         name = (std::string("NEW_2"));
//         LOG("Inserting %s\n", name.c_str());
//         st_freq_mem m2{0, (uint16_t)(n + 1), (uint64_t)150, FM, name.c_str()};
//         save(m2);
//         log_file();
//         name = (std::string("NEW_3"));
//         LOG("Inserting %s\n", name.c_str());
//         st_freq_mem m3{0, (uint16_t)(n + 1), (uint64_t)500, FM, name.c_str()};
//         save(m3);
//         log_file();
//     }
// }

void fix_db() {
    INIT_OR_ABORT()
    if (!db_file->line_count()) {
        return;
    }
    LOG("Fixing memory database. Count: %d\n", db_file->line_count());
    auto all = get_all();
    LOG("Fixing memory database. Found %d non empty lines\n", all.size());
    build_sorted_database(all);
    LOG("Fixed memory database. Count: %d\n", db_file->line_count());
}

void init_memory_mode() {
    uint64_t f = radio::get_frequency();
    DIRECTION d = f == radio::get_max_frequency() ? BACKWARDS : FORWARD;

    st_freq_mem mem = find_closest(f, d);

    if (mem.freq) {
        freq_memory::set(mem);
    } else {
        using namespace status;
        pop_alert(Level::ERROR, "Frequency memory empty");
    }
}

// Initialize file buffer
bool init_file_buffer() {

    if (sdcard_info.status != sdcard_STATUS::Mounted) {
        return false;
    }

    if (db_file) {
        return true; // Already initialized
    }

    auto result = std::unique_ptr<io::FileWrapper<>>(new io::FileWrapper<>());

    // Mute to avoid SD card EMI. There's a TODO in some place to address this (new board design)
    main_board::set_mute(GPIO_PIN_SET);
    status::pop_alert(status::INFO, "Initializing memory");
    view_manager::currentView->paint();
    bool res = true;

    if (result->load(FREQ_MEMORY_FILE, true)) {
        db_file = std::move(result);

        sdcard_signal.add(NULL, [](void *, const void *) {
            if (sdcard_info.status != sdcard_STATUS::Mounted) {
                db_file.reset();
            }
        });

        radio::band_signal.add(NULL, [](void *, const void *) {
            if (memory_mode_on()) {
                init_memory_mode();
            }
        });

        if (memory_mode_on() && curr_index < 0) {
            init_memory_mode();
        }

    } else {
        res = false;
    }

    main_board::set_mute(GPIO_PIN_RESET);
    return res;
}

// Get frequency memory entry by index (line number)
st_freq_mem get_by_index(int index) {

    INIT_OR_ABORT({});

    if (index < 0) {
        return {};
    }

    if (index >= static_cast<int>(db_file->line_count())) {
        return {}; // Index out of bounds
    }

    // Use the new get_line_content method which handles newlines properly
    std::string line = db_file->get_line(index);

    st_freq_mem m = deserialize_freq_mem(line.c_str());

    return m;
}

auto extract_freq_func = [](const std::string &line) {
    st_freq_mem m = deserialize_freq_mem(line.c_str());
    return m.freq;
};

void find_in_freq_range(uint64_t freq_min, uint64_t freq_max, std::vector<st_freq_mem> &out_memories, const std::vector<FREQ_TYPE> &types) {

    //  LOG("find_in_freq_range %d, %d\n", freq_min, freq_max);
    INIT_OR_ABORT()
    out_memories.clear();

    std::vector<uint32_t> line_numbers;

    FRESULT res = db_file->find_range(freq_min, freq_max, extract_freq_func, line_numbers, MAX_RETRIEVED_ITEMS_PER_RANGE);

    if (res != FR_OK) {
        // TODO: Remove this once this is stable
        // fix_db();
        return;
    }

    out_memories.reserve(line_numbers.size());

    if (!line_numbers.empty()) {
        uint32_t min_line = *std::min_element(line_numbers.begin(), line_numbers.end());
        uint32_t max_line = *std::max_element(line_numbers.begin(), line_numbers.end());

        std::vector<std::string> lines = db_file->get_lines_range(min_line, max_line + 1);

        for (auto line : lines) {
            if (!line.empty()) {
                auto item = deserialize_freq_mem(line.c_str());
                if (types.empty() || std::find(types.begin(), types.end(), item.type) != types.end()) {
                    out_memories.push_back(item);
                }
            }
        }
    }
}

void get_band_modes_in_range(uint64_t freq_min, uint64_t freq_max, std::vector<st_freq_mem> &out_memories) {

    find_in_freq_range(freq_min, freq_max, out_memories, {BAND_START, BAND_END});
}

// Get total number of frequency memory entries
int get_freq_mem_count() {
    INIT_OR_ABORT(0)
    return static_cast<int>(db_file->line_count());
}

// st_freq_mem temporary register
st_freq_mem tempFreqMem;
char tempFreqBuf[] = "00 000 000 000";

using namespace Menu;

// A function to save the edited data record
void save_target() {
    char *ptr;
    removePunct(tempFreqBuf);
    tempFreqMem.freq = strtol(tempFreqBuf, &ptr, 10);

    save(tempFreqMem);
}

/**
 * Retrieves the index of a stored frequency by frequency and mode
 */
int find_index(st_freq_mem &data) {
    INIT_OR_ABORT(-1)

    //   LOG("Finding index for %s (%d) id:%d\n", data.name, data.freq, data.id);
    auto res = db_file->binary_search_first(data.freq, extract_freq_func, io::FindMode::EQ);

    if (res.is_error()) {
        return -1;
    }

    uint32_t line_pos = *res;

    // Linear search to find exact item (many can have the same frequency if they have different types)
    for (int32_t pos = line_pos; pos >= 0 && pos < get_freq_mem_count(); pos++) {
        //    LOG("find_index: get by index %d\n", pos);
        st_freq_mem mem = get_by_index(pos);
        if (mem == data) {
            return pos;
        }
    }

    return -1;
}

void save(st_freq_mem &mem) {

    auto res = db_file->binary_search_first(mem.freq, extract_freq_func, io::GTE);

    uint32_t line_pos = *res;

    std::string serialized = serialize_freq_mem(mem);

    if (line_pos >= db_file->line_count()) {

        // Append at end
        mem.id = line_pos;
        db_file->append_line(serialized);

    } else {

        // Check if frequency already exists

        std::string existing_line = db_file->get_line(line_pos);
        st_freq_mem existing = deserialize_freq_mem(existing_line.c_str());

        if (existing.freq == mem.freq) {
            // Update existing item
            mem.id = line_pos;
            db_file->replace_line(line_pos, serialized);

        } else {

            // Insert at correct position - shift remaining lines
            std::vector<std::string> remaining_lines;
            for (uint32_t i = line_pos; i < db_file->line_count(); ++i) {
                remaining_lines.push_back(db_file->get_line(i));
            }

            db_file->replace_line(line_pos, serialized);
            for (size_t i = 0; i < remaining_lines.size(); ++i) {
                uint32_t ix = line_pos + 1 + i;
                mem.id = ix;
                if (ix >= db_file->line_count()) {
                    db_file->append_line(remaining_lines[i]);
                } else {
                    db_file->replace_line(ix, remaining_lines[i]);
                }
            }
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
        save(tempFreqMem);
    };
    view_manager::push((View *)&view_manager::keyboardView);
}

result edit_freq_name(eventMask, navNode &) {
    view_manager::keyboardView.set_text(tempFreqMem.name);
    view_manager::keyboardView.set_label("Name");
    view_manager::keyboardView.set_size(FREQ_MEM_NAME_SIZE);
    view_manager::keyboardView.on_changed = [](char *str) {
        strncpy(tempFreqMem.name, str, FREQ_MEM_NAME_SIZE);
        save_target();
    };
    view_manager::push((View *)&view_manager::keyboardView);
    return proceed;
}

/* Start frequency edition */
result edit_freq(eventMask, navNode &) {
    Menu::open_keypad<uint64_t>(
        tempFreqMem.freq, "Hz", "Frequency", 0, false,
        [](uint64_t v) {
            tempFreqMem.freq = v;
            char buf[16];
            format_long(tempFreqMem.freq, buf);
            sprintf(tempFreqBuf, "%s", buf);
            save_target();
        },
        radio::get_min_frequency(), radio::get_max_frequency());

    return proceed;
}

/* Deletes the radio station with a particular index */
void del_freq(int i) {
    if (i >= 0 && i < get_freq_mem_count()) {
        db_file->delete_line(i);
        using namespace status;
        pop_alert(Level::INFO, "Deleted");
    } else {
        using namespace status;
        pop_alert(Level::ERROR, "Error deleting");
    }
}

/* Retrieves a pointer to the next or previous radio station of that of the current index, by frequency order */
st_freq_mem next_prev(bool next) {
    static st_freq_mem found_mem; // Static to return pointer

    INIT_OR_ABORT({})

    int16_t step = next ? 1 : -1;
    int count = get_freq_mem_count();
    uint16_t ix = curr_index >= 0 ? constrain(curr_index + step, 0, count) : 0;
    // LOG("next_prev: %d, getting index  %d\n", next, ix);
    return get_by_index(ix);
}

/* Tune the radio to the frequency of a station */
void set(st_freq_mem &mem) {
    if (radio::set_frequency(mem.freq)) {
        int ix = find_index(mem);
        if (ix >= 0) {
            curr_index = ix;
        }
        main_board::set_modulation_mode(mem.mode, false);
    } else {
        using namespace status;
        if (radio::get_curr_freq_band() != radio::find_band(mem.freq)) {

            pop_alert(Level::ERROR, "Out of currently selected band");
        } else {

            pop_alert(Level::ERROR, "radio::set_frequency() was false");
        }
    }
}

/* Sets the next frequency in a given direction */
void set_next_prev(DIRECTION d, FREQ_TYPE t) {
    if (memory_mode_on()) {
        if (curr_index >= 0) {
            st_freq_mem curr_mem = get_by_index(curr_index);
            st_freq_mem mem = find_closest(curr_mem.freq, d, t);
            if (mem == curr_mem) {
                mem = find_closest(curr_mem.freq + (d == FORWARD ? 1 : -1), d, t);
            }
            if (mem.freq) {
                freq_memory::set(mem);
            }
        } else {
            // Shoudn't happen
        }
    }
}

/* Finds the closest station to a given frequency and direction */
st_freq_mem find_closest(uint64_t f, DIRECTION direction, FREQ_TYPE t) {
    static st_freq_mem found_mem; // Static to return pointer

    INIT_OR_ABORT({})

    int count = get_freq_mem_count();
    io::FindMode mode = direction == FORWARD ? io::GTE : io::LTE;

    //   LOG("find_closest to %d mode: %d, type: %d, type: %d\n", f, mode);
    auto res = db_file->binary_search_first(f, extract_freq_func, mode);

    if (res.is_error()) {
        return {};
    }

    int32_t start_pos = *res;
    if (start_pos >= count) {
        return {};
    }

    // Now a linear scan from binary search position (to discard unwanted types)
    int32_t step = (direction == FORWARD) ? 1 : -1;
    for (int32_t pos = start_pos + step; pos >= 0 && pos < count; pos += step) {
        st_freq_mem mem = get_by_index(pos);
        if (t == ALL || mem.type == t) {
            return mem;
        }
    }

    return {};
}

bool memory_mode_on() {
    return config.memory_mode;
}

uint8_t toggle_memory_mode() {
    bool memory_mode = (memory_mode_on() == 0 ? 1 : 0);

    if (memory_mode) {
        init_memory_mode();
    } else {
        curr_index = -1;
    }

    config.memory_mode = memory_mode;

    return 0;
}

st_freq_mem get_current() {
    if (curr_index) {
        return get_by_index(curr_index);
    }

    return {};
}

labelPrompt freqNameMenu((const char *)"Name", tempFreqMem.name, edit_freq_name, enterEvent, noStyle);
labelPrompt freqEditMenu((const char *)"Frequency", tempFreqBuf, edit_freq, enterEvent, noStyle);

optionsPrompt<MODULATION_MODE> modulationModeMenu((const char *)"Modulation", modulation_options, config.modulation,
                                                  sizeof(modulation_options) / sizeof(modulation_options[0]), [](MODULATION_MODE) {
                                                      save_target();
                                                  });

MENU(freqMemEditMenu, "Frequency edit", doNothing, noEvent, wrapStyle, OBJ(freqNameMenu), OBJ(modulationModeMenu), OBJ(freqEditMenu));

result freqMemorySelectedEvent(eventMask e, navNode &nav);

FreqMemoryMenu freqMemMenu("Frequency memory", 512, nullptr, freqMemEditMenu, freqMemorySelectedEvent, (eventMask)(enterEvent | exitEvent));

menu_actions_st &get_actions() {
    static menu_action_st menu_actions[] = {get_navigation_actions().actions[Menu::UP],
                                            get_navigation_actions().actions[Menu::DOWN],
                                            {"Delete",
                                             []() {
                                                 if (freqMemMenu.curr_ix >= 0) {
                                                     del_freq(freqMemMenu.curr_ix);
                                                 }
                                             }},
                                            {},
                                            get_navigation_actions().actions[Menu::ENTER],
                                            get_navigation_actions().actions[Menu::BACK]};

    static menu_actions_st actions = {menu_actions, sizeof(menu_actions) / sizeof(menu_action_st)};

    return actions;
}
/*
 * This will be called whenever an entry is selected in the frequency memory
 * It copies the currently selected index st_freq_mem in the temporary struct
 */
result freqMemorySelectedEvent(eventMask e, navNode &nav) {
    if (nav.target == &freqMemMenu && freqMemMenu.curr_ix >= 0) { // Only if we are on memory menu
        tempFreqMem = get_by_index(freqMemMenu.curr_ix);

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
        ((MenuWidget *)view_manager::mainView.Menu())->set_quick_actions(&get_actions());

    } else if (e == Menu::exitEvent) {
        // Remove context actions
        ((MenuWidget *)view_manager::mainView.Menu())->set_default_quick_actions();
    }

    return proceed;
}

} // namespace freq_memory
