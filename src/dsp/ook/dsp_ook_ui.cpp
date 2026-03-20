//
// OOK Transmitter UI
//

#include "dsp_ook_ui.h"
#include "config.h"
#include "dsp/dsp.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_tasks.h"
#include "dsp/ook/dsp_ook_processor.h"
#include "dsp/ook/ook_brute_presets.h"
#include "dsp/ook/ook_task.h"
#include "dsp/ook/ook_widget.h"
#include "main_board.h"
#include "menuBase.h"
#include "radio.h"
#include "status.h"
#include "types.h"
#include "ui/main_view.h"
#include "ui/menu.h"
#include "ui/menu_frequency.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace dspOOKUI {

using namespace Menu;

static bool stopped = true;
static SignalToken signal_token;

// ----------------------------------------------------------------------------
// Common UI state
// ----------------------------------------------------------------------------

static uint8_t ook_mode = 0; // 0: Manual, 1: Brute

Menu::menu_option_st<uint8_t> mode_options[] = {{"Manual", 0}, {"Brute", 1}};

// ----------------------------------------------------------------------------
// Manual OOK parameters
// ----------------------------------------------------------------------------

static uint32_t manual_mark_duration_us = 500;  // Carrier ON duration per "1" bit
static uint32_t manual_space_duration_us = 500; // Carrier OFF duration per "0" bit
static uint32_t manual_pause_us = 10000;        // Gap between repetitions
static uint16_t manual_repetitions = 3;
static bool manual_loop_enabled = false;

// Sequence as editable text (0s and 1s)
static const char *constMEM binaryChars MEMMODE = "01";
static const char *constMEM binaryMask[1] MEMMODE = {binaryChars};
static constexpr size_t MAX_SEQ_LEN = 128;
static char sequence_buf[MAX_SEQ_LEN + 1] = "10101010";

// Parsed sequence (updated from sequence_buf)
static std::vector<uint8_t> parsed_sequence;

// ----------------------------------------------------------------------------
// Brute-force OOK parameters (PortaPack-style)
// ----------------------------------------------------------------------------

static uint8_t brute_protocol = static_cast<uint8_t>(OOKBruteProtocol::CAME_12);
static uint32_t brute_start_code = 0;
static uint32_t brute_stop_code = 4095;
static uint32_t brute_step = 1;
static uint32_t brute_chip_duration_us = 333;
static uint32_t brute_pause_us = 0;
static uint16_t brute_repetitions = 2;
static bool brute_wrap_enabled = false;

static std::vector<uint8_t> brute_preview_sequence;

Menu::menu_option_st<uint8_t> brute_proto_options[] = {
    {"CAME 12", static_cast<uint8_t>(OOKBruteProtocol::CAME_12)},
    {"CAME 24", static_cast<uint8_t>(OOKBruteProtocol::CAME_24)},
    {"NICE 12", static_cast<uint8_t>(OOKBruteProtocol::NICE_12)},
    {"NICE 24", static_cast<uint8_t>(OOKBruteProtocol::NICE_24)},
    {"Holtek HT12", static_cast<uint8_t>(OOKBruteProtocol::HOLTEK_HT12)},
    {"Princeton 24", static_cast<uint8_t>(OOKBruteProtocol::PRINCETON_24)},
};

// ----------------------------------------------------------------------------
// Widget
// ----------------------------------------------------------------------------

static OOKWidget ook_w({{DISPLAY_X_PIXELS / 2, MENU_START_Y, DISPLAY_X_PIXELS / 2, INFO_HEIGHT - 6}, &lcd, "ook"});

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

static void normalize_sequence_buf() {
    // Keep a single trailing blank so the menu editor can extend the sequence
    // one bit at a time (by editing the blank into 0/1).
    size_t len = strlen(sequence_buf);

    while (len > 0 && sequence_buf[len - 1] == ' ') {
        sequence_buf[len - 1] = '\0';
        len--;
    }

    if (len < MAX_SEQ_LEN) {
        sequence_buf[len] = ' ';
        sequence_buf[len + 1] = '\0';
    }
}

static void parse_sequence() {
    parsed_sequence.clear();
    for (size_t i = 0; i < MAX_SEQ_LEN && sequence_buf[i] != '\0'; i++) {
        char c = sequence_buf[i];
        if (c == '1') {
            parsed_sequence.push_back(1);
        } else if (c == '0') {
            parsed_sequence.push_back(0);
        }
    }
}

static void normalize_brute_range() {
    const auto *preset = ook_brute_get_preset(static_cast<OOKBruteProtocol>(brute_protocol));
    if (!preset) {
        brute_start_code = 0;
        brute_stop_code = 0;
        return;
    }

    const uint32_t max_code = ook_brute_max_code(*preset);

    if (brute_start_code > max_code) {
        brute_start_code = max_code;
    }
    if (brute_stop_code > max_code) {
        brute_stop_code = max_code;
    }

    if (brute_start_code > brute_stop_code) {
        brute_start_code = brute_stop_code;
    }
    if (brute_stop_code < brute_start_code) {
        brute_stop_code = brute_start_code;
    }

    if (brute_step == 0) {
        brute_step = 1;
    }
}

static void rebuild_brute_preview() {
    brute_preview_sequence.clear();

    const auto *preset = ook_brute_get_preset(static_cast<OOKBruteProtocol>(brute_protocol));
    if (!preset) {
        return;
    }

    normalize_brute_range();
    (void)ook_brute_build_sequence(*preset, brute_start_code, brute_preview_sequence);
}

static void update_widget() {
    if (ook_mode == 0) {
        ook_w.set_sequence(&parsed_sequence);
        ook_w.set_timing(manual_mark_duration_us, manual_space_duration_us);
    } else {
        rebuild_brute_preview();
        ook_w.set_sequence(&brute_preview_sequence);
        ook_w.set_timing(brute_chip_duration_us, brute_chip_duration_us);
    }

    ook_w.set_dirty();
}

static bool pending_ook_config = false;

static void apply_processor_config(st_dsp_params *task_info) {
    if (!dsp_task) {
        return;
    }

    auto *proc = (DspOOKProcessor *)dsp_task->get_processor();
    if (!proc) {
        return;
    }

    const uint32_t sr = task_info ? task_info->sample_rate : dsp_task->info.sample_rate;

    if (ook_mode == 0) {
        // Manual
        proc->set_config(0, manual_mark_duration_us, manual_space_duration_us, manual_pause_us, sr);
        proc->set_sequence(parsed_sequence);
        proc->set_loop(manual_loop_enabled);
        proc->set_repetitions(manual_repetitions);

        ook_w.set_sequence(&parsed_sequence);
        ook_w.set_timing(manual_mark_duration_us, manual_space_duration_us);

    } else {
        // Brute
        normalize_brute_range();

        proc->set_config(0, brute_chip_duration_us, brute_chip_duration_us, brute_pause_us, sr);
        proc->set_bruteforce(static_cast<OOKBruteProtocol>(brute_protocol), brute_start_code, brute_stop_code, brute_step);
        proc->set_loop(brute_wrap_enabled);
        proc->set_repetitions(brute_repetitions);

        rebuild_brute_preview();
        ook_w.set_sequence(&brute_preview_sequence);
        ook_w.set_timing(brute_chip_duration_us, brute_chip_duration_us);
    }

    ook_w.set_task_status(&dsp_task->info);
    ook_w.set_dirty();
}

// Frequency edit
static result on_freq_updated();
static menu_frequency::FreqEditField freqEdit((callback)on_freq_updated);

static void on_freq_signal(void *, const void *args) {
    radio::st_freq_event event = *((radio::st_freq_event *)args);
    if (event.event == radio::AFTER_UPDATE) {
        freqEdit.set_frequency(event.frequency);
    }
}

static void on_event(st_dsp_params *, st_dsp_params *task_info) {

    switch (task_info->status) {
        case DSP_STATUS_RUNNING:
            stopped = false;
            if (pending_ook_config) {
                apply_processor_config(task_info);
                pending_ook_config = false;
            }
            break;
        case DSP_STATUS_PENDING:
            stopped = false;
            break;
        case DSP_STATUS_STOPPED:
        case DSP_STATUS_STOPPING:
            stopped = true;
            break;
    }
}

static void configure_and_start() {

    if (ook_mode == 0) {
        // Manual
        normalize_sequence_buf();
        parse_sequence();

        if (parsed_sequence.empty()) {
            using namespace status;
            pop_alert(Level::ERROR, "Empty OOK sequence");
            return;
        }

    } else {
        // Brute
        const auto *preset = ook_brute_get_preset(static_cast<OOKBruteProtocol>(brute_protocol));
        if (!preset) {
            using namespace status;
            pop_alert(Level::ERROR, "Invalid brute preset");
            return;
        }

        normalize_brute_range();
        rebuild_brute_preview();

        if (brute_preview_sequence.empty()) {
            using namespace status;
            pop_alert(Level::ERROR, "Invalid brute sequence");
            return;
        }
    }

    update_widget();

    pending_ook_config = true;
    dsp_start(
        []() {
            auto task = std::make_unique<OOKTask>();
            return task;
        },
        on_event);

    if (dsp_task) {
        ook_w.set_task_status(&dsp_task->info);
    }
}

static result change_dsp_status(eventMask e) {
    if (e == enterEvent) {
        if (!stopped) {
            dsp_stop();
        } else {
            configure_and_start();
        }
    }
    return proceed;
}

static result on_freq_updated() {
    if (!stopped) {
        using namespace status;
        pop_alert(Level::ERROR, "Stop TX first");
        return proceed;
    }

    radio::set_frequency(freqEdit.get_frequency());
    return proceed;
}

static void on_brute_protocol_changed() {
    const auto *preset = ook_brute_get_preset(static_cast<OOKBruteProtocol>(brute_protocol));
    if (!preset) {
        return;
    }

    brute_chip_duration_us = preset->chip_duration_us;
    brute_repetitions = preset->default_repeat;
    brute_step = 1;

    brute_start_code = 0;
    brute_stop_code = ook_brute_max_code(*preset);

    update_widget();
}

static result on_sequence_updated(eventMask) {
    normalize_sequence_buf();
    parse_sequence();
    update_widget();
    return proceed;
}

static void on_manual_timing_changed() {
    update_widget();
}

static void on_brute_params_changed() {
    normalize_brute_range();
    update_widget();
}

static result on_menu_event(eventMask e) {

    switch (e) {
        case enterEvent:
            signal_token = radio::freq_signal.add(NULL, on_freq_signal);
            freqEdit.set_frequency(radio::get_frequency());
            dsp_set_real_time(true);

            normalize_sequence_buf();
            parse_sequence();
            view_manager::mainView.add_child(&ook_w);
            update_widget();
            ook_w.set_visible(true);
            menu_size(DISPLAY_X_PIXELS / 2, INFO_HEIGHT);
            break;

        case exitEvent:
            if (stopped) {
                radio::freq_signal.remove(signal_token);
                dsp_set_real_time(false);
                ook_w.set_visible(false);
                view_manager::mainView.remove_child(&ook_w);
                menu_size(DISPLAY_X_PIXELS, INFO_HEIGHT);
            } else {
                using namespace status;
                pop_alert(Level::ERROR, "Transmitting!");
                return quit;
            }
            break;

        default:
            break;
    }
    return proceed;
}

// ----------------------------------------------------------------------------
// Menu items
// ----------------------------------------------------------------------------

Menu::optionsPrompt<uint8_t> modeMenu((const char *)"Mode", mode_options, ook_mode, sizeof(mode_options) / sizeof(mode_options[0]),
                                      [](uint8_t) {
                                          update_widget();
                                      });

// Manual prompts
numberPrompt<uint32_t> markDurationMenu((const char *)"Mark:", &manual_mark_duration_us, 0, ' ', '.', "us",
                                        [](uint32_t) {
                                            on_manual_timing_changed();
                                        },
                                        10, 1000000, 10, 100);

numberPrompt<uint32_t> spaceDurationMenu((const char *)"Space:", &manual_space_duration_us, 0, ' ', '.', "us",
                                         [](uint32_t) {
                                             on_manual_timing_changed();
                                         },
                                         10, 1000000, 10, 100);

numberPrompt<uint32_t> manualPauseMenu((const char *)"Pause:", &manual_pause_us, 0, ' ', '.', "us",
                                       [](uint32_t) {
                                       },
                                       0, 10000000, 100, 1000);

numberPrompt<uint16_t> manualRepetitionsMenu((const char *)"Reps:", &manual_repetitions, 0, ' ', '.', "",
                                             [](uint16_t) {
                                             },
                                             1, 10000, 1, 10);

// Brute prompts
Menu::optionsPrompt<uint8_t> bruteProtocolMenu((const char *)"Attack", brute_proto_options, brute_protocol,
                                               sizeof(brute_proto_options) / sizeof(brute_proto_options[0]),
                                               [](uint8_t) {
                                                   on_brute_protocol_changed();
                                               });

numberPrompt<uint32_t> bruteStartMenu((const char *)"Start:", &brute_start_code, 0, ' ', '.', "",
                                      [](uint32_t) {
                                          on_brute_params_changed();
                                      },
                                      0, 0xFFFFFFFFu, 1, 10);

numberPrompt<uint32_t> bruteStopMenu((const char *)"Stop:", &brute_stop_code, 0, ' ', '.', "",
                                     [](uint32_t) {
                                         on_brute_params_changed();
                                     },
                                     0, 0xFFFFFFFFu, 1, 10);

numberPrompt<uint32_t> bruteStepMenu((const char *)"Step:", &brute_step, 0, ' ', '.', "",
                                     [](uint32_t) {
                                         on_brute_params_changed();
                                     },
                                     1, 0xFFFFFFFFu, 1, 10);

numberPrompt<uint32_t> bruteChipMenu((const char *)"Chip:", &brute_chip_duration_us, 0, ' ', '.', "us",
                                     [](uint32_t) {
                                         on_brute_params_changed();
                                     },
                                     10, 5000, 10, 100);

numberPrompt<uint32_t> brutePauseMenu((const char *)"Pause:", &brute_pause_us, 0, ' ', '.', "us",
                                      [](uint32_t) {
                                          on_brute_params_changed();
                                      },
                                      0, 10000000, 100, 1000);

numberPrompt<uint16_t> bruteRepetitionsMenu((const char *)"Reps:", &brute_repetitions, 0, ' ', '.', "",
                                            [](uint16_t) {
                                                on_brute_params_changed();
                                            },
                                            1, 10000, 1, 10);

#ifdef __clang__
#ifndef typeof
#define typeof __typeof__
#endif
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

TOGGLE(manual_loop_enabled, manualLoopToggle, "Loop: ", doNothing, noEvent, noStyle, VALUE("On", true, doNothing, noEvent),
       VALUE("Off", false, doNothing, noEvent));

TOGGLE(brute_wrap_enabled, bruteWrapToggle, "Wrap: ", doNothing, noEvent, noStyle, VALUE("On", true, doNothing, noEvent),
       VALUE("Off", false, doNothing, noEvent));

MENU(ookManualMenu, "OOK Manual", doNothing, noEvent, noStyle, EDIT("Seq:", sequence_buf, binaryMask, on_sequence_updated, updateEvent, noStyle),
     OBJ(markDurationMenu), OBJ(spaceDurationMenu), OBJ(manualPauseMenu), OBJ(manualRepetitionsMenu), SUBMENU(manualLoopToggle))

MENU(ookBruteMenu, "OOK Brute", doNothing, noEvent, noStyle, OBJ(bruteProtocolMenu), OBJ(bruteStartMenu), OBJ(bruteStopMenu), OBJ(bruteStepMenu),
     OBJ(bruteChipMenu), OBJ(brutePauseMenu), OBJ(bruteRepetitionsMenu), SUBMENU(bruteWrapToggle))

MENU(ookMenu, "OOK Transmitter", on_menu_event, (eventMask)(enterEvent | exitEvent), noStyle, OP("Start / Stop", change_dsp_status, enterEvent),
     OBJ(modeMenu), SUBMENU(ookManualMenu), SUBMENU(ookBruteMenu), OBJ(freqEdit))

#ifdef __clang__
#pragma clang diagnostic pop
#endif

} // namespace dspOOKUI
