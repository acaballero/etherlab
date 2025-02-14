//
// Created by Angel Dust on 19/12/2022.
//

#include "scanner_ui.h"

#include <io/file_factory.h>
#include <sys/_stdint.h>
#include "menuBase.h"
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

void scanner_callback(void *, void *args) {
    scanner::st_scanner_info *config = (scanner::st_scanner_info *)args;
    scanner_config = *config;
}

Menu::result on_menu_event(Menu::eventMask e) {

    switch (e) {

        case Menu::enterEvent:

            if (scanner_config.freq_min == 0) {
                scanner_config = {.freq_min = radio::get_frequency() - 200000,
                                  .freq_max = radio::get_frequency() + 200000,
                                  .freq_step = 2500,
                                  .squelch = config.squelch_level,
                                  .pause_ms = 2000,
                                  .period_s = 1,
                                  .save_found = false,
                                  .direction = FORWARD,
                                  .mode = scanner::scanner_config.mode};
            }

            // Subscribe to scanner signals
            scanner::signal.add(NULL, scanner_callback);

            configure();

            break;

        case Menu::exitEvent:
            break;
    }

    return Menu::proceed;
}

using namespace Menu;

// TODO: Add 'Band' mode to automatically select the current band frequency span
menu_option_st<scanner::SCANNER_MODE> mode_options[] = {
    {"Custom", scanner::SCANNER_MODE_CUSTOM}, {"Band", scanner::SCANNER_MODE_BAND}, {"List", scanner::SCANNER_MODE_LIST}};

optionsPrompt<scanner::SCANNER_MODE> modeMenu((const char *)"Direction", mode_options, scanner_config.mode, sizeof(mode_options) / sizeof(mode_options[0]),
                                              [](scanner::SCANNER_MODE) { configure(); });

// TODO: Add 'Band' mode to automatically select the current band frequency span
menu_option_st<DIRECTION> direction_options[] = {{"Backwards", BACKWARDS}, {"Forward", FORWARD}};

optionsPrompt<DIRECTION> directionMenu((const char *)"Direction", direction_options, scanner_config.direction,
                                       sizeof(direction_options) / sizeof(direction_options[0]), [](DIRECTION) { configure(); });

TOGGLE(scanner_config.status, scanEnableToggle, "Status: ", configure, enterEvent, noStyle, VALUE("On", scanner::SCANNER_STATUS_RUNNING, doNothing, noEvent),
       VALUE("Off", scanner::SCANNER_STATUS_STOPPED, doNothing, noEvent));

Menu::numberPrompt<uint32_t> freqStepMenu((const char *)"Step", &scanner_config.freq_step, 0, ' ', '.', "Hz", [](uint32_t) { configure(); }, 1000, 1000000,
                                          1000, 10000);

Menu::numberPrompt<uint16_t> freqPeriodMenu((const char *)"Period", &scanner_config.period_s, 0, ' ', '.', "s", [](uint16_t) { configure(); }, 1, 60000, 1, 10);

Menu::numberPrompt<uint32_t> freqPauseDelay((const char *)"Pause delay", &scanner_config.pause_ms, 0, ' ', '.', "ms", [](uint32_t) { configure(); }, 0, 10000,
                                            100, 1000);

MENU(menuScan, "Scan", on_menu_event, (Menu::eventMask)(enterEvent | exitEvent), noStyle, SUBMENU(scanEnableToggle), OBJ(directionMenu), OBJ(freqStepMenu),
     OBJ(freqPeriodMenu), OBJ(freqPauseDelay), OBJ(modeMenu), OBJ(freqEditMin), OBJ(freqEditMax));
} // namespace scanner_ui
