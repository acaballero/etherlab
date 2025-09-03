//
// Created by Angel Dust on 16/04/2021.
//
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

result dsp_compressor_set(eventMask) {
    if (dsp::dsp_config.audio_compressor_enabled) {
        compressorThresholdMenu.enable();
    } else {
        compressorThresholdMenu.disable();
    }
    dsp_restart();
    return proceed;
}

TOGGLE(dsp::dsp_config.audio_compressor_enabled, toggleDSPCompressor, "Audio compressor: ", doNothing, noEvent, noStyle, //,doExit,enterEvent,noStyle
       VALUE("On", true, dsp_compressor_set, noEvent), VALUE("Off", false, dsp_compressor_set, noEvent));

TOGGLE(dsp::dsp_config.deemphasis_enabled, toggleFMDeemph, "FM Deemph: ", doNothing, noEvent, noStyle, //,doExit,enterEvent,noStyle
       VALUE("On", true, dsp_compressor_set, noEvent), VALUE("Off", false, dsp_compressor_set, noEvent));

TOGGLE(dsp::dsp_config.audio_bpf_enabled, toggleAudioBPF, "Audio BPF: ", doNothing, noEvent, noStyle, //,doExit,enterEvent,noStyle
       VALUE("On", true, dsp_compressor_set, noEvent), VALUE("Off", false, dsp_compressor_set, noEvent));

result open_aprs(eventMask) {
    Menu::menu_exit();
    view_manager::open_aprs();
    return proceed;
}

/* TODO: Disable SD card related functionality if card is not enabled */
MENU(menuDSP, "DSP", doNothing, anyEvent, noStyle, SUBMENU(dspCaptureUI::captureMenu), SUBMENU(dspReplayUI::replayMenu),
     SUBMENU(dspSignalGeneratorUI::signalGeneratorMenu), OP("APRS", open_aprs, enterEvent), SUBMENU(toggleDSP), SUBMENU(toggleAGC), SUBMENU(toggleAudioBPF),
     SUBMENU(toggleFMDeemph), SUBMENU(toggleDSPCompressor), OBJ(compressorThresholdMenu), OBJ(dspBandwidthMenu), OBJ(dspWFMMaxDev), OBJ(dspFMMaxDev));

} // namespace dsp_ui
