//
// OOK Transmitter UI
//

#include "dsp_ook_ui.h"
#include "dsp/dsp.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_tasks.h"
#include "dsp/ook/ook_task.h"
#include "dsp/ook/dsp_ook_processor.h"
#include "dsp/ook/ook_widget.h"
#include "main_board.h"
#include "menuBase.h"
#include "radio.h"
#include "status.h"
#include "types.h"
#include "ui/main_view.h"
#include "ui/menu.h"
#include "ui/menu_frequency.h"
#include "config.h"

#include <cstring>
#include <vector>

namespace dspOOKUI {

using namespace Menu;

static bool stopped = true;
static SignalToken signal_token;

// OOK timing parameters
static uint32_t mark_duration_us = 500;  // Carrier ON duration per "1" bit
static uint32_t space_duration_us = 500; // Carrier OFF duration per "0" bit
static uint32_t pause_us = 10000;        // Gap between repetitions
static uint16_t repetitions = 3;
static bool loop_enabled = false;

// Sequence as editable text (0s and 1s)
static const char *constMEM binaryChars MEMMODE = "01";
static const char *constMEM binaryMask[1] MEMMODE = {binaryChars};
static constexpr size_t MAX_SEQ_LEN = 128;
static char sequence_buf[MAX_SEQ_LEN + 1] = "10101010";

// Parsed sequence (updated from sequence_buf)
static std::vector<uint8_t> parsed_sequence;

// Widget
static OOKWidget ook_w({{DISPLAY_X_PIXELS / 2, MENU_START_Y, DISPLAY_X_PIXELS / 2, INFO_HEIGHT - 6}, &lcd, "ook"});

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

// Parse the text buffer into a vector of 0/1 values.
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

// Push current timing params to the widget so the waveform redraws correctly
static void update_widget() {
    ook_w.set_sequence(&parsed_sequence);
    ook_w.set_timing(mark_duration_us, space_duration_us);
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

    uint32_t sr = task_info ? task_info->sample_rate : dsp_task->info.sample_rate;

    proc->set_config(0, mark_duration_us, space_duration_us, pause_us, sr);
    proc->set_sequence(parsed_sequence);
    proc->set_loop(loop_enabled);
    proc->set_repetitions(repetitions);

    ook_w.set_sequence(&parsed_sequence);
    ook_w.set_timing(mark_duration_us, space_duration_us);
    ook_w.set_task_status(&dsp_task->info);
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
    normalize_sequence_buf();
    parse_sequence();

    if (parsed_sequence.empty()) {
        using namespace status;
        pop_alert(Level::ERROR, "Empty OOK sequence");
        return;
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
    return proceed;
}

// Called when the sequence text is edited
static result on_sequence_updated(eventMask) {
    normalize_sequence_buf();
    parse_sequence();
    update_widget();
    return proceed;
}

// Called when any timing parameter changes
static void on_timing_changed() {
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

// Menu items

numberPrompt<uint32_t> markDurationMenu((const char *)"Mark:", &mark_duration_us, 0, ' ', '.', "us",
                                        [](uint32_t) {
                                            on_timing_changed();
                                        },
                                        10, 1000000, 10, 100);

numberPrompt<uint32_t> spaceDurationMenu((const char *)"Space:", &space_duration_us, 0, ' ', '.', "us",
                                         [](uint32_t) {
                                             on_timing_changed();
                                         },
                                         10, 1000000, 10, 100);

numberPrompt<uint32_t> pauseMenu((const char *)"Pause:", &pause_us, 0, ' ', '.', "us",
                                 [](uint32_t) {
                                 },
                                 0, 10000000, 100, 1000);

numberPrompt<uint16_t> repetitionsMenu((const char *)"Reps:", &repetitions, 0, ' ', '.', "",
                                       [](uint16_t) {
                                       },
                                       1, 10000, 1, 10);

#ifdef __clang__
#ifndef typeof
#define typeof __typeof__
#endif
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

TOGGLE(loop_enabled, loopToggle, "Loop: ", doNothing, noEvent, noStyle, VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent));

MENU(ookMenu, "OOK Transmitter", on_menu_event, (eventMask)(enterEvent | exitEvent), noStyle, OP("Start / Stop", change_dsp_status, enterEvent),
     EDIT("Seq:", sequence_buf, binaryMask, on_sequence_updated, updateEvent, noStyle), OBJ(markDurationMenu), OBJ(spaceDurationMenu), OBJ(pauseMenu),
     OBJ(repetitionsMenu), SUBMENU(loopToggle), OBJ(freqEdit))

#ifdef __clang__
#pragma clang diagnostic pop
#endif

} // namespace dspOOKUI
