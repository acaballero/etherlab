//
// Created by Angel Dust on 02/07/2024.
//

#include <io/file_factory.h>
#include "dsp_signal_generator_ui.h"
#include "../dsp_common.h"
#include "../dsp_tasks.h"
#include "../signal_generator/signal_generator_task.h"
#include "../dsp.h"
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

    int8_t gain;
    int8_t pulse_duty = 50;
    uint32_t baseband_frequency = 10000;
    uint32_t modulation_frequency = 1000;
    int command = DSP_COMMAND_START;
    SignalToken signal_token;

    void on_freq_signal(void *thisptr, void *args) {
        radio::st_freq_event event = *((radio::st_freq_event *) args);
        if (event.event == radio::AFTER_UPDATE) {
            // TODO: Update signal
        }
    }

    void on_event(st_dspStatus *status) {

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

    Menu::result set_signal_params(Menu::eventMask e) {
        DspSignalGeneratorProcessor *processor = ((DspSignalGeneratorProcessor *) processors[DSP_TASK_SIGNAL_GENERATOR]);
        processor->set_config(baseband_frequency, modulation_frequency, pulse_duty, config.fft.sample_rate, config.hw.dac_offset);
        return Menu::proceed;
    }

    Menu::result change_dsp_status(Menu::eventMask e) {

        if (e == Menu::activateEvent) {
            DSP_COMMAND nextCommand = command == DSP_COMMAND_START ? DSP_COMMAND_STOP : DSP_COMMAND_START;
            dsp_command({(DSP_COMMAND) nextCommand, DSP_TASK_SIGNAL_GENERATOR}, on_event);

            set_signal_params(e);
        }

        return Menu::proceed;
    }

    Menu::result on_menu_event(Menu::eventMask e) {

        SignalGeneratorTask *task = ((SignalGeneratorTask *) tasks[DSP_TASK_SIGNAL_GENERATOR]);

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
                    status::handleError(status::ST_ERROR, "Playing!");
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

    result change_gain(eventMask e) {
        set_tx_gain_db(gain);
        return proceed;
    }

    TOGGLE(command, signalGeneratorToggle, "Command: ", change_dsp_status, anyEvent, noStyle//,doExit,enterEvent,noStyle
    , VALUE("Stop", DSP_COMMAND_STOP, change_dsp_status, anyEvent),
           VALUE("Start", DSP_COMMAND_START, change_dsp_status, anyEvent)
    )

    Menu::result on_freq_updated(); // Forward declaration

    menu_frequency::FreqEditField freqEdit((Menu::callback) on_freq_updated);

    Menu::result on_freq_updated() {
        radio::set_frequency(freqEdit.get_frequency());
        return Menu::proceed;
    }

    MENU(signalGeneratorMenu, "Signal generator", on_menu_event, (eventMask) (enterEvent | exitEvent | selBlurEvent),
         noStyle,
         SUBMENU(signalGeneratorToggle),
         FIELD(config.hw.dac_offset, "DAC offset:", "", 0, 2000, 1, 0, doNothing, noEvent, noStyle),
         FIELD(baseband_frequency, "Baseband freq.:", "", 0, FFT_BANDWIDTH, 500, 0, set_signal_params, exitEvent, noStyle),
         FIELD(modulation_frequency, "Modulation freq.:", "", 0, 5000, 100, 0, set_signal_params, exitEvent, noStyle),
         FIELD(pulse_duty, "Pulse duty:", "", 1, 100, 1, 0, set_signal_params, exitEvent, noStyle),
         FIELD(gain, "Gain:", " dB", DSP_MIN_TX_GAIN_DB, DSP_MAX_TX_GAIN_DB, 1, 0, change_gain, exitEvent, noStyle),
         FIELD(config.fft.span, "Span", "Hz.", FFT_MIN_SPAN, FFT_MAX_SPAN, 10000, 0, set_sampling_params, anyEvent,
               noStyle),
         OBJ(freqEdit),
         EXIT("<Back")
    )
}

