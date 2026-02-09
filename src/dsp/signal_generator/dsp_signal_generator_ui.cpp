//
// Created by Angel Dust on 02/07/2024.
//

#include <io/file_factory.h>
#include <sys/_stdint.h>
#include "class/audio/audio.h"
#include "dsp/blocks/signal_generator.h"
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
#include "dsp/capture/capture_task.h"

namespace dspSignalGeneratorUI {

RF_DIRECTION mode = RF_DIRECTION_TX;
bool stopped = true;
SignalToken signal_token;

// Pre-declare
void set_signal_params();

void on_freq_signal(void *thisptr, const void *args) {
    radio::st_freq_event event = *((radio::st_freq_event *)args);
    if (event.event == radio::AFTER_UPDATE) {
        // TODO: Update signal
    }
}

void on_event(st_dsp_params *status) {

    switch (status->status) {

        case DSP_STATUS_RUNNING:
        case DSP_STATUS_PENDING:
            stopped = false;
            set_signal_params();
            break;
        case DSP_STATUS_STOPPED:
            stopped = true;
            break;
    }
}

Menu::numberPrompt<int8_t> pulseDutyMenu((const char *)"Pulse duty:", &dsp::dsp_config.test_signal.pulse_duty, 0, ' ', '.', "%",
                                         [](int8_t) {
                                             set_signal_params();
                                         },
                                         0, 100, 1, 10);

void update_menu() {
    if (dsp::dsp_config.test_signal.shape == SIGNAL_SHAPE_PULSE) {
        pulseDutyMenu.enable();
    } else {
        pulseDutyMenu.disable();
    }
}
void set_signal_params() {
    auto task = (SignalGeneratorTask *)(dsp_task.get());
    auto *processor = (DspSignalGeneratorProcessor *)task->get_processor();

    if (dsp::dsp_config.test_signal.shape == SIGNAL_SHAPE_PULSE) {
        processor->set_config(dsp::dsp_config.test_signal.baseband_frequency, dsp::dsp_config.test_signal.modulation_frequency,
                              dsp::dsp_config.test_signal.pulse_duty, config.fft.sample_rate);

    } else {
        processor->set_config(dsp::dsp_config.test_signal.baseband_frequency, dsp::dsp_config.test_signal.modulation_frequency,
                              (SIGNAL_SHAPE)dsp::dsp_config.test_signal.shape, config.fft.sample_rate);
    }

    update_menu();

    task->mode = mode;
}

Menu::result change_dsp_status(Menu::eventMask e) {

    if (e == Menu::enterEvent) {

        if (!stopped) {
            dsp_stop();
        } else {
            dsp_start(dsp::DSP_TASK_SIGNAL_GENERATOR, on_event);
            set_signal_params();
        }
    }

    return Menu::proceed;
}

Menu::result on_menu_event(Menu::eventMask e) {
    auto *task = (SignalGeneratorTask *)dsp_task.get();

    switch (e) {
        case Menu::selBlurEvent:
            // If the task is finished, reset it
            if (task->info.stop_ms) {
                task->info.stop_ms = 0;
            }
            break;

        case Menu::enterEvent:
            signal_token = radio::freq_signal.add(NULL, on_freq_signal);
            dsp_set_real_time(true);
            update_menu();
            break;

        case Menu::exitEvent:
            if (stopped) {
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

TOGGLE(mode, modeToggle, "Mode: ", doNothing, anyEvent,
       noStyle, //,doExit,enterEvent,noStyle       ,
       VALUE("RX", RF_DIRECTION_RX, doNothing, anyEvent), VALUE("TX", RF_DIRECTION_TX, doNothing, anyEvent))

Menu::result on_freq_updated(); // Forward declaration

menu_frequency::FreqEditField freqEdit((Menu::callback)on_freq_updated);

Menu::result on_freq_updated() {
    radio::set_frequency(freqEdit.get_frequency());
    return Menu::proceed;
}

Menu::menu_option_st<uint8_t> shape_options[] = {{"Sine", SIGNAL_SHAPE_SIN},
                                                 {"Saw down", SIGNAL_SHAPE_SAW_DOWN},
                                                 {"Saw up", SIGNAL_SHAPE_SAW_UP},
                                                 {"Triangle", SIGNAL_SHAPE_TRI},
                                                 {"Pulse", SIGNAL_SHAPE_PULSE}};

Menu::numberPrompt<int8_t> gainMenu((const char *)"Gain", &dsp::dsp_config.gain, 0, ' ', '.', "dB",
                                    [](int8_t v) {
                                        dsp::set_gain_db(v);
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
                                                     1, DSP_BANDWIDTH, 1, 10);

Menu::optionsPrompt<uint8_t> shapeMenu((const char *)"Shape", shape_options, dsp::dsp_config.test_signal.shape,
                                       sizeof(shape_options) / sizeof(shape_options[0]), [](uint8_t) {
                                           set_signal_params();
                                       });

Menu::numberPrompt<float> dacAmpBalanceMenu((const char *)"DAC amplitude balance", &config.hw.dac_amp_balance, 2, ' ', '.', nullptr,
                                            [](float) {

                                            },
                                            0.5, 1.5, 0.01, 0.1);

MENU(signalGeneratorMenu, "Signal generator", on_menu_event, (eventMask)(enterEvent | exitEvent | selBlurEvent), noStyle,
     OP("Start / Stop", change_dsp_status, enterEvent), SUBMENU(modeToggle),
     FIELD(config.hw.dac_offset, "DAC offset:", "", 0, 3000, 1, 0, doNothing, noEvent, noStyle),
     FIELD(config.hw.dac_off_balance, "DAC offset  balance:", "", -1000, 1000, 1, 0, doNothing, noEvent, noStyle), OBJ(dacAmpBalanceMenu),
     OBJ(basebandFrequencyMenu), OBJ(modulationFrequencyMenu), OBJ(shapeMenu), OBJ(pulseDutyMenu), OBJ(gainMenu), OBJ(freqEdit))
} // namespace dspSignalGeneratorUI
