//
// Created by Angel Dust on 19/12/2022.
//

#include "scanner_ui.h"

#include <io/file_factory.h>
#include "ui/menu.h"
#include "status.h"
#include "scanner.h"
#include "menu_frequency.h"

namespace scanner_ui {

    scanner::st_scanner_info scanner_config;

    Menu::result configue_scanner(Menu::eventMask e); // Forward declaration

    menu_frequency::FreqEditField freqEdit_min("Start freq", (Menu::callback) configue_scanner);
    menu_frequency::FreqEditField freqEdit_max("Stop freq", (Menu::callback) configue_scanner);

    void configure() {

        switch (scanner_config.mode) {
            case scanner::SCANNER_MODE_CUSTOM:
                freqEdit_min.enable();
                freqEdit_max.enable();
                break;
            case scanner::SCANNER_MODE_BAND:
                freqEdit_min.disable();
                freqEdit_max.disable();
                break;
            case scanner::SCANNER_MODE_LIST:
                freqEdit_min.disable();
                freqEdit_max.disable();
                break;
        }

        scanner_config.freq_min = freqEdit_min.get_frequency();
        scanner_config.freq_max = freqEdit_max.get_frequency();
        freqEdit_min.set_max_frequency(scanner_config.freq_max);
        freqEdit_max.set_min_frequency(scanner_config.freq_min);

        scanner::configure(scanner_config);
    }

    void scanner_callback(void *thisptr, void *args) {
        scanner::st_scanner_info* config = (scanner::st_scanner_info *)args;
        scanner_config = *config;
    }

    Menu::result on_menu_event(Menu::eventMask e) {

        switch (e) {
            case Menu::enterEvent:

                if (scanner_config.freq_min == 0) {
                    scanner_config = {
                            .freq_min=radio::get_frequency()-200000,
                            .freq_max=radio::get_frequency()+200000,
                            .freq_step=2500,
                            .squelch=config.squelch_level,
                            .pause_ms=2000,
                            .period_s=1,
                            .direction=FORWARD,
                            .mode=scanner::scanner_config.mode
                    };
                }

                freqEdit_min.set_frequency(scanner_config.freq_min);
                freqEdit_max.set_frequency(scanner_config.freq_max);

                scanner::signal.add(NULL, scanner_callback);

                configure();

                break;
            case Menu::exitEvent:
                break;
        }
        return Menu::proceed;
    }

    using namespace Menu;

    result configue_scanner(eventMask e) {
        configure();
        return proceed;
    }

    prompt *directionValues[] = {
            new Menu::menuValue<DIRECTION>("Backwards", BACKWARDS),
            new Menu::menuValue<DIRECTION>("Forward", FORWARD)
    };

    prompt *modeValues[] = {
            new Menu::menuValue<scanner::SCANNER_MODE>("Custom", scanner::SCANNER_MODE_CUSTOM),
            new Menu::menuValue<scanner::SCANNER_MODE>("Band", scanner::SCANNER_MODE_BAND),
            new Menu::menuValue<scanner::SCANNER_MODE>("List", scanner::SCANNER_MODE_LIST)
    };

    // TODO: Add 'Band' mode to automatically select the current band frequency span
    Menu::select<DIRECTION> &directionMenu =
            *new Menu::select<DIRECTION>("Direction:",
                                         scanner_config.direction,
                                         sizeof(directionValues) / sizeof(prompt *),
                                         directionValues, configue_scanner, exitEvent);

    Menu::select<scanner::SCANNER_MODE> &modeMenu =
            *new Menu::select<scanner::SCANNER_MODE>("Mode:",
                                                     scanner_config.mode,
                                                     sizeof(modeValues) / sizeof(prompt *),
                                                     modeValues, configue_scanner, exitEvent);

    TOGGLE(scanner_config.status, scanEnableToggle, "Status: ", configue_scanner, enterEvent, noStyle//,doExit,enterEvent,noStyle
    , VALUE("On", scanner::SCANNER_STATUS_RUNNING, doNothing, noEvent), VALUE("Off", scanner::SCANNER_STATUS_STOPPED, doNothing, noEvent)
    );

    MENU(menuScan, "Scan", on_menu_event, (Menu::eventMask) (enterEvent | exitEvent), noStyle,
         SUBMENU(scanEnableToggle),
         SUBMENU(directionMenu),
         FIELD(scanner_config.freq_step, "Step:", " Hz", 1000, 1000000, 1000, 0, configue_scanner, enterEvent, noStyle),
         FIELD(scanner_config.period_s, "Period:", " s", 1, 60000, 1, 0, configue_scanner, enterEvent, noStyle),
         FIELD(scanner_config.pause_ms, "Scan pause:", " ms", 0, 10000, 1000, 0, configue_scanner, enterEvent, noStyle),
         SUBMENU(modeMenu),
         OBJ(freqEdit_min),
         OBJ(freqEdit_max),
         EXIT("<Back")
    );
}
