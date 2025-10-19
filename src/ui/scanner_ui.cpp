//
// Created by Angel Dust on 19/12/2022.
//

#include "scanner_ui.h"

#include <io/file_factory.h>

#include "Signal.h"
#include "dsp/fft/fft.h"
#include "menuBase.h"
#include "radio.h"
#include "s_strength.h"
#include "types.h"
#include "ui/menu.h"
#include "status.h"
#include "scanner.h"
#include "menu_frequency.h"
#include "menu_prompts.h"

namespace scanner_ui {

scanner::st_scanner_info scanner_config;

Menu::numberPrompt<uint64_t> freqEditMin((const char *)"Freq. from", &scanner_config.freq_min, 0, ' ', '.', "Hz", nullptr, (uint64_t)config.f_min,
                                         (uint64_t)config.f_max);
Menu::numberPrompt<uint64_t> freqEditMax((const char *)"Freq. to", &scanner_config.freq_max, 0, ' ', '.', "Hz", nullptr, (uint64_t)config.f_min,
                                         (uint64_t)config.f_max);

void configure() {

    switch (scanner_config.mode) {
        case scanner::SCANNER_MODE_CUSTOM:
            freqEditMin.enable();
            freqEditMax.enable();
            break;
        case scanner::SCANNER_MODE_BAND:
            freqEditMin.disable();
            freqEditMax.disable();
            break;
        case scanner::SCANNER_MODE_LIST:
            freqEditMin.disable();
            freqEditMax.disable();
            break;
    }

    freqEditMin.max = scanner_config.freq_max;
    freqEditMax.min = scanner_config.freq_min;

    scanner::configure(scanner_config);
}

void scanner_callback(void *, const void *args) {
    scanner::st_scanner_info *config = (scanner::st_scanner_info *)args;
    scanner_config = *config;
}

SignalToken signal_token;

Menu::result on_menu_event(Menu::eventMask e) {

    switch (e) {

        case Menu::enterEvent:

            if (scanner_config.freq_min == 0) {
                scanner_config = {//.freq_min = radio::bands[radio::get_band()].freq_start,
                                  //.freq_max = radio::bands[radio::get_band()].freq_end,
                                  .freq_min = radio::get_frequency() - 200000,
                                  .freq_max = radio::get_frequency() + 200000,
                                  .freq_step = radio::if_filters[radio::if_filter].bandwidth,
                                  .squelch = 0,
                                  .pause_ms = 2000,
                                  .period_s = 1,
                                  .save_found = false,
                                  .direction = FORWARD,
                                  .mode = scanner::scanner_config.mode};
            }

            scanner_config.squelch = max2(config.squelch_level, sstrength::db_to_s_strength(fft::fft_noise_floor_db) + 3);

            // Subscribe to scanner signals
            signal_token = scanner::signal.add(NULL, scanner_callback);

            configure();

            break;

        case Menu::exitEvent:

            scanner::signal.remove(signal_token);
            break;
    }

    return Menu::proceed;
}

using namespace Menu;

// TODO: Add 'Band' mode to automatically select the current band frequency span
menu_option_st<scanner::SCANNER_MODE> mode_options[] = {
    {"Custom", scanner::SCANNER_MODE_CUSTOM}, {"Band", scanner::SCANNER_MODE_BAND}, {"List", scanner::SCANNER_MODE_LIST}};

menu_option_st<uint32_t> filter_options[] = {{radio::IFFilterNames[radio::IF_FILTER_3KHZ], radio::if_filters[radio::IF_FILTER_3KHZ].bandwidth},
                                             {radio::IFFilterNames[radio::IF_FILTER_15KHZ], radio::if_filters[radio::IF_FILTER_15KHZ].bandwidth},
                                             {radio::IFFilterNames[radio::IF_FILTER_150KHZ], radio::if_filters[radio::IF_FILTER_150KHZ].bandwidth}};

optionsPrompt<scanner::SCANNER_MODE> modeMenu((const char *)"Direction", mode_options, scanner_config.mode, sizeof(mode_options) / sizeof(mode_options[0]),
                                              [](scanner::SCANNER_MODE) {
                                                  configure();
                                              });

// TODO: Add 'Band' mode to automatically select the current band frequency span
menu_option_st<DIRECTION> direction_options[] = {{"Backwards", BACKWARDS}, {"Forward", FORWARD}};

optionsPrompt<DIRECTION> directionMenu((const char *)"Direction", direction_options, scanner_config.direction,
                                       sizeof(direction_options) / sizeof(direction_options[0]), [](DIRECTION) {
                                           configure();
                                       });

TOGGLE(scanner_config.status, scanEnableToggle, "Status: ", configure, enterEvent, noStyle, VALUE("On", scanner::SCANNER_STATUS_RUNNING, doNothing, noEvent),
       VALUE("Off", scanner::SCANNER_STATUS_STOPPED, doNothing, noEvent));

TOGGLE(scanner_config.save_found, scanSaveToggle, "Save: ", configure, enterEvent, noStyle, VALUE("Yes", true, doNothing, noEvent),
       VALUE("No", false, doNothing, noEvent));

// In alalog scan, the step must be equal to the IF filter bandwidth so the expected signals lie in the middle of the passband. Otherwise, we'd have to work
// hard to discern the center frequency when a signal is detected and also when moving away from the last detected signal.
Menu::optionsPrompt<uint32_t> freqStepMenu((const char *)"Step", filter_options, scanner_config.freq_step, 3, [](uint16_t) {
    configure();
});

Menu::numberPrompt<uint16_t> freqPeriodMenu((const char *)"Period", &scanner_config.period_s, 0, ' ', '.', "s",
                                            [](uint16_t) {
                                                configure();
                                            },
                                            1, 60000, 1, 10);

Menu::numberPrompt<uint32_t> freqPauseDelay((const char *)"Pause delay", &scanner_config.pause_ms, 0, ' ', '.', "ms",
                                            [](uint32_t) {
                                                configure();
                                            },
                                            0, 10000, 100, 1000);

Menu::numberPrompt<float> squelchMenu((const char *)"S-level", &scanner_config.squelch, 0, ' ', '.', "",
                                      [](uint32_t) {
                                          configure();
                                      },
                                      0, 9, 1, 0.1);

MENU(menuScan, "Scan", on_menu_event, (Menu::eventMask)(enterEvent | exitEvent), noStyle, SUBMENU(scanEnableToggle), OBJ(directionMenu),
     SUBMENU(scanSaveToggle), OBJ(freqStepMenu), OBJ(freqPeriodMenu), OBJ(freqPauseDelay), OBJ(modeMenu), OBJ(squelchMenu), OBJ(freqEditMin), OBJ(freqEditMax));
} // namespace scanner_ui
