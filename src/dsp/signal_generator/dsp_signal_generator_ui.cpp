//
// Created by Angel Dust on 02/07/2024.
//

#include <io/file_factory.h>
#include "dsp/fft/fft_types.h"
#include "dsp_signal_generator_ui.h"
#include "../dsp_common.h"
#include "../dsp_tasks.h"
#include "../signal_generator/signal_generator_task.h"
#include "../dsp.h"
#include "hw/board/board_v2.h"
#include "menuBase.h"
#include "types.h"
#include "ui/main_view.h"
#include "ui/sd_filepicker_menu.h"
#include "signal_generator_widget.h"
#include "ui/menu.h"
#include "ui/menu_frequency.h"
#include "radio.h"
#include "status.h"
#include "dsp/signal_generator/dsp_signal_generator_processor.h"
#include "dsp/dsp_processors.h"
#include "dsp/capture/capture_task.h"

namespace dspSignalGeneratorUI {

RF_DIRECTION mode;
int command = DSP_COMMAND_START;
SignalToken signal_token;

void on_freq_signal(void *thisptr, const void *args) {
    radio::st_freq_event event = *((radio::st_freq_event *)args);
    if (event.event == radio::AFTER_UPDATE) {
        // TODO: Update signal
    }
}

void on_event(st_dsp_status *status) {

    switch (status->status) {

        case DSP_STATUS_RUNNING:
        case DSP_STATUS_PENDING:
            command = DSP_COMMAND_STOP;
            break;
        case DSP_STATUS_STOPPED:
            command = DSP_COMMAND_START;
            break;
    }
}

void set_signal_params() {
    DspSignalGeneratorProcessor *processor = ((DspSignalGeneratorProcessor *)processors[dsp::DSP_TASK_SIGNAL_GENERATOR]);
    processor->set_config(dsp::dsp_config.test_signal.baseband_frequency, dsp::dsp_config.test_signal.modulation_frequency,
                          dsp::dsp_config.test_signal.pulse_duty, config.fft.sample_rate, config.hw.dac_offset);

    // Tasks parameters. Essentially, the IF direction
    SignalGeneratorTask *task = ((SignalGeneratorTask *)dsp::tasks[dsp::DSP_TASK_SIGNAL_GENERATOR]);
    task->mode = mode;
}

Menu::result change_dsp_status(Menu::eventMask e) {

    if (e == Menu::activateEvent) {
        DSP_COMMAND nextCommand = command == DSP_COMMAND_START ? DSP_COMMAND_STOP : DSP_COMMAND_START;
        dsp_command({(DSP_COMMAND)nextCommand, dsp::DSP_TASK_SIGNAL_GENERATOR}, on_event);
        dsp::set_tx_gain_db(dsp::dsp_config.gain);

        set_signal_params();
    }

    return Menu::proceed;
}

Menu::result on_menu_event(Menu::eventMask e) {

    SignalGeneratorTask *task = ((SignalGeneratorTask *)dsp::tasks[dsp::DSP_TASK_SIGNAL_GENERATOR]);

    switch (e) {

        case Menu::selBlurEvent:
            // If the task is finished, reset it
            if (task->status.stop_ms) {
                task->status.stop_ms = 0;
            }
            break;

        case Menu::enterEvent:
            signal_token = radio::freq_signal.add(NULL, on_freq_signal);
            dsp_set_real_time(true);
            break;

        case Menu::exitEvent:
            if (task->status.status == DSP_STATUS_STOPPED) {
                radio::freq_signal.remove(signal_token);
                dsp_set_real_time(false);
            } else {
                status::pop_alert(status::ERROR, "Playing!");
                return Menu::quit; // Cancel exit
            }
            break;
    }

    return Menu::proceed;
}

using namespace Menu;

result set_sampling_params(eventMask e) {
    fft_config(config.fft.span);
    return proceed;
}

TOGGLE(command, signalGeneratorToggle, "Command: ", change_dsp_status, anyEvent,
       noStyle, //,doExit,enterEvent,noStyle       ,
       VALUE("Stop", DSP_COMMAND_STOP, change_dsp_status, anyEvent), VALUE("Start", DSP_COMMAND_START, change_dsp_status, anyEvent))

TOGGLE(mode, modeToggle, "Mode: ", doNothing, anyEvent,
       noStyle, //,doExit,enterEvent,noStyle       ,
       VALUE("RX", RF_DIRECTION_RX, doNothing, anyEvent), VALUE("TX", RF_DIRECTION_TX, doNothing, anyEvent))

Menu::result on_freq_updated(); // Forward declaration

menu_frequency::FreqEditField freqEdit((Menu::callback)on_freq_updated);

Menu::result on_freq_updated() {
    radio::set_frequency(freqEdit.get_frequency());
    return Menu::proceed;
}

Menu::numberPrompt<int8_t> gainMenu((const char *)"Gain", &dsp::dsp_config.gain, 0, ' ', '.', "dB",
                                    [](int8_t v) {
                                        dsp::set_tx_gain_db(v);
                                    },
                                    DSP_MIN_TX_GAIN_DB, DSP_MAX_TX_GAIN_DB, 1, 5);

Menu::numberPrompt<uint32_t> basebandFrequencyMenu((const char *)"Baseband freq:", &dsp::dsp_config.test_signal.baseband_frequency, 0, ' ', '.', "Hz",
                                                   [](uint32_t) {
                                                       set_signal_params();
                                                   },
                                                   10, DSP_BANDWIDTH, 10, 100);

Menu::numberPrompt<uint32_t> modulationFrequencyMenu((const char *)"Modulation freq:", &dsp::dsp_config.test_signal.modulation_frequency, 0, ' ', '.', "Hz",
                                                     [](uint32_t) {
                                                         set_signal_params();
                                                     },
                                                     10, DSP_BANDWIDTH, 10, 100);

Menu::numberPrompt<int8_t> pulseDutyMenu((const char *)"Pulse duty:", &dsp::dsp_config.test_signal.pulse_duty, 0, ' ', '.', "%",
                                         [](int8_t) {
                                             set_signal_params();
                                         },
                                         0, 100, 1, 10);

MENU(signalGeneratorMenu, "Signal generator", on_menu_event, (eventMask)(enterEvent | exitEvent | selBlurEvent), noStyle, SUBMENU(signalGeneratorToggle),
     SUBMENU(modeToggle), FIELD(config.hw.dac_offset, "DAC offset:", "", 0, 2000, 1, 0, doNothing, noEvent, noStyle), OBJ(basebandFrequencyMenu),
     OBJ(modulationFrequencyMenu), OBJ(pulseDutyMenu), OBJ(gainMenu),
     FIELD(config.fft.span, "Span", "Hz.", FFT_MIN_SPAN, FFT_MAX_SPAN, 10000, 0, set_sampling_params, anyEvent, noStyle), OBJ(freqEdit))
} // namespace dspSignalGeneratorUI
