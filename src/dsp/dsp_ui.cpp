//
// Created by Angel Dust on 16/04/2021.
//
#include "aprs/aprs_ui.h"
#include "ook/dsp_ook_ui.h"
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
#include "ui/menu_prompts.hpp"
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
                                          10000, 150000, 5000, 10000);

Menu::numberPrompt<uint32_t> dspFMMaxDev((const char *)"FM max. deviation", &config.dsp.fm_max_deviation, 0, ' ', '.', "Hz",
                                         [](uint32_t) {
                                             dsp_restart();
                                         },
                                         2000, 5000, 100, 1000);

static void apply_cw_preset(uint8_t preset, bool restart = true);

result apply_dsp_changes(eventMask) {
    if (dsp::dsp_config.audio_compressor_enabled) {
        compressorThresholdMenu.enable();
    } else {
        compressorThresholdMenu.disable();
    }

    apply_cw_preset(dsp::dsp_config.cw_decode_preset, false);
    config.dsp = dsp::dsp_config;
    dsp_restart();

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

TOGGLE(dsp::dsp_config.decode_cw, toggleDecodeCW, "Decode CW: ", doNothing, noEvent, noStyle,
       VALUE("On", true, apply_dsp_changes, noEvent), VALUE("Off", false, apply_dsp_changes, noEvent));

static void apply_cw_preset(uint8_t preset, bool restart) {
    switch (preset) {
        case 0: // Noisy / weak
            dsp::dsp_config.cw_decode_bw_hz = 1000;
            dsp::dsp_config.cw_goertzel_snr_db = 4;
            dsp::dsp_config.cw_transition_min_dot_percent = 18;
            dsp::dsp_config.cw_letter_gap_mult_x10 = 18;
            dsp::dsp_config.cw_word_gap_mult_x10 = 55;
            break;
        case 2: // Strong / clean
            dsp::dsp_config.cw_decode_bw_hz = 600;
            dsp::dsp_config.cw_goertzel_snr_db = 8;
            dsp::dsp_config.cw_transition_min_dot_percent = 30;
            dsp::dsp_config.cw_letter_gap_mult_x10 = 24;
            dsp::dsp_config.cw_word_gap_mult_x10 = 68;
            break;
        case 1:
        default: // Balanced
            dsp::dsp_config.cw_decode_bw_hz = 700;
            dsp::dsp_config.cw_goertzel_snr_db = 6;
            dsp::dsp_config.cw_transition_min_dot_percent = 25;
            dsp::dsp_config.cw_letter_gap_mult_x10 = 22;
            dsp::dsp_config.cw_word_gap_mult_x10 = 62;
            break;
    }

    if (restart) {
        dsp_restart();
    }
}

Menu::menu_option_st<dsp::CwDecodeAlgorithm> cw_algorithm_options[] = {{"Goertzel", dsp::CW_DECODE_GOERTZEL},
                                                                         {"Envelope", dsp::CW_DECODE_ENVELOPE},
                                                                         {"Mayhem", dsp::CW_DECODE_MAYHEM}};

Menu::optionsPrompt<dsp::CwDecodeAlgorithm> cwAlgorithmMenu((const char *)"Algorithm", cw_algorithm_options, dsp::dsp_config.cw_decode_algorithm,
                                                            sizeof(cw_algorithm_options) / sizeof(cw_algorithm_options[0]), [](dsp::CwDecodeAlgorithm v) {
                                                                dsp::dsp_config.cw_decode_algorithm = v;
                                                                apply_dsp_changes(enterEvent);
                                                            });

Menu::menu_option_st<uint8_t> cw_preset_options[] = {{"Noisy", 0}, {"Balanced", 1}, {"Clean", 2}};

Menu::optionsPrompt<uint8_t> cwPresetMenu((const char *)"Preset", cw_preset_options, dsp::dsp_config.cw_decode_preset,
                                          sizeof(cw_preset_options) / sizeof(cw_preset_options[0]), [](uint8_t v) {
                                              dsp::dsp_config.cw_decode_preset = v;
                                              apply_cw_preset(v, true);
                                          });

MENU(cwDecoderMenu, "CW decoder", doNothing, anyEvent, noStyle, SUBMENU(toggleDecodeCW), OBJ(cwAlgorithmMenu), OBJ(cwPresetMenu));

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
     SUBMENU(dspSignalGeneratorUI::signalGeneratorMenu), OP("APRS", open_aprs, enterEvent), OP("Radiosonde", open_radiosonde, enterEvent),
     SUBMENU(dspOOKUI::ookMenu), SUBMENU(toggleDSP), SUBMENU(toggleAGC), SUBMENU(toggleBasebandEcho), SUBMENU(cwDecoderMenu), SUBMENU(toggleAudioBPF), SUBMENU(toggleFMDeemph),
     SUBMENU(toggleDSPCompressor), OBJ(compressorThresholdMenu), OBJ(dspBandwidthMenu), OBJ(dspWFMMaxDev), OBJ(dspFMMaxDev));

} // namespace dsp_ui
