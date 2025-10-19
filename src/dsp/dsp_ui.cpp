//
// Created by Angel Dust on 16/04/2021.
//
#include "aprs/aprs_ui.h"
#include "config.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_tasks.h"
#include "dsp/radiosonde/radiosonde_ui.hpp"
#include "dsp/receive/receive_task.h"
#include "ui/menu.h"
#include "dsp_ui.h"
#include "main_board.h"
#include "menuIo.h"
#include "menuDefs.h"
#include "ui/menu_prompts.h"
#include "stdio.h"
#include "dsp.h"

namespace dsp_ui {

using namespace Menu;

bool dsp_enabled = !ISANALOG;

result toggle_dsp(eventMask) {
    main_board::toggle_dsp();
    return proceed;
}

TOGGLE(dsp_enabled, toggleDSP, "DSP receiver: ", doNothing, noEvent, noStyle, //,doExit,enterEvent,noStyle
       VALUE("On", true, toggle_dsp, noEvent), VALUE("Off", false, toggle_dsp, noEvent));

TOGGLE(config.dsp.agc_enabled, toggleAGC, "DSP AGC: ", doNothing, noEvent, noStyle, //,doExit,enterEvent,noStyle
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent));

Menu::numberPrompt<int32_t> compressorThresholdMenu((const char *)"Compressor threshold", &dsp::dsp_config.audio_compressor_threshold, 0, ' ', '.', "dB",
                                                    [](int32_t) {
                                                        dsp_restart();
                                                    },
                                                    -50, 30, 1, 10);

Menu::numberPrompt<uint32_t> dspBandwidthMenu((const char *)"DSP bandwidth", &config.fft.bw, 0, ' ', '.', "Hz",
                                              [](uint32_t) {
                                                  dsp_restart();
                                              },
                                              DSP_BANDWIDTH / 2, DSP_BANDWIDTH * 2, 1000, 10000);

Menu::numberPrompt<uint32_t> dspWFMMaxDev((const char *)"WFM max. deviation", &config.dsp.wideband_fm_max_deviation, 0, ' ', '.', "Hz",
                                          [](uint32_t) {
                                              dsp_restart();
                                          },
                                          10000, 100000, 5000, 10000);

Menu::numberPrompt<uint32_t> dspFMMaxDev((const char *)"FM max. deviation", &config.dsp.fm_max_deviation, 0, ' ', '.', "Hz",
                                         [](uint32_t) {
                                             dsp_restart();
                                         },
                                         2000, 5000, 100, 1000);

result apply_dsp_changes(eventMask) {
    if (dsp::dsp_config.audio_compressor_enabled) {
        compressorThresholdMenu.enable();
    } else {
        compressorThresholdMenu.disable();
    }

    ((ReceiveTask *)dsp::tasks[dsp::DSP_TASK_RECEIVE])->set_baseband_echo(dsp::dsp_config.baseband_echo);

    if (!ISANALOG) {
        dsp_restart();
    }
    return proceed;
}

TOGGLE(dsp::dsp_config.audio_compressor_enabled, toggleDSPCompressor, "Audio compressor: ", doNothing, noEvent, noStyle, //,doExit,enterEvent,noStyle
       VALUE("On", true, apply_dsp_changes, noEvent), VALUE("Off", false, apply_dsp_changes, noEvent));

TOGGLE(dsp::dsp_config.deemphasis_enabled, toggleFMDeemph, "FM Deemph: ", doNothing, noEvent, noStyle, //,doExit,enterEvent,noStyle
       VALUE("On", true, apply_dsp_changes, noEvent), VALUE("Off", false, apply_dsp_changes, noEvent));

TOGGLE(dsp::dsp_config.audio_bpf_enabled, toggleAudioBPF, "Audio BPF: ", doNothing, noEvent, noStyle, //,doExit,enterEvent,noStyle
       VALUE("On", true, apply_dsp_changes, noEvent), VALUE("Off", false, apply_dsp_changes, noEvent));

TOGGLE(dsp::dsp_config.baseband_echo, toggleBasebandEcho, "Baseband echo: ", doNothing, noEvent, noStyle, //,doExit,enterEvent,noStyle
       VALUE("On", true, apply_dsp_changes, noEvent), VALUE("Off", false, apply_dsp_changes, noEvent));

result open_aprs(eventMask) {
    Menu::close();
    view_manager::open_app(std::make_unique<dsp_ui::APRSView>(Rect{0, MENU_START_Y - 50, DISPLAY_X_PIXELS, METERS_HEIGHT + 80}));
    return proceed;
}

result open_radiosonde(eventMask) {
    Menu::close();
    view_manager::open_app(std::make_unique<dsp_ui::RadiosondeView>(Rect{0, MENU_START_Y - 50, DISPLAY_X_PIXELS, METERS_HEIGHT + 80}));
    return proceed;
}

/* TODO: Disable SD card related functionality if card is not enabled */
MENU(menuDSP, "DSP", doNothing, anyEvent, noStyle, SUBMENU(dspCaptureUI::captureMenu), SUBMENU(dspReplayUI::replayMenu),
     SUBMENU(dspSignalGeneratorUI::signalGeneratorMenu), OP("APRS", open_aprs, enterEvent), OP("Radiosonde", open_radiosonde, enterEvent), SUBMENU(toggleDSP),
     SUBMENU(toggleAGC), SUBMENU(toggleBasebandEcho), SUBMENU(toggleAudioBPF), SUBMENU(toggleFMDeemph), SUBMENU(toggleDSPCompressor),
     OBJ(compressorThresholdMenu), OBJ(dspBandwidthMenu), OBJ(dspWFMMaxDev), OBJ(dspFMMaxDev));

} // namespace dsp_ui
