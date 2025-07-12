
#include "menu.h"
#include "dsp/dsp.h"
#include "dsp/dsp_common.h"
#include "frequency_memory_ui.h"
#include "Display_afb.h"
#include "config.h"
#include "dsp/fft/fft_types.h"
#include "lcd.h"
#include "main_view.h"
#include "../../lib/Menu/src/menuIO/chainStream.h"
#include "../../lib/Menu/src/menuIO/stringIn.h"
#include "../../lib/Menu/src/plugin/userMenu.h"
#include "menuBase.h"
#include "mixer.h"
#include "types.h"
#include "ui/menuILI9431Out.h"
#include "dsp/dsp_ui.h"
#include "ui/menu_actions.h"
#include "ui/scanner_ui.h"
#include "dsp/fft/fft_ui.h"
#include "main.h"
#include "main_board.h"
#include "fatfs/fatfs.h"
#include "radio.h"
#include "rf_coupler.h"
#include "../../lib/utils/utils.hpp"
#include "ui/ui_types.h"
#include "view_manager.h"
#include "s_strength.h"
#include <cstddef>
#include <cstring>
#include <memory>
#include "settings.h"
#include "menu_options.h"
#include "menu_prompts.h"
#include "standby.h"
#include "ui/lock_view.h"

namespace Menu {

// Character validators for the frequency memories
const char *constMEM alphaNum MEMMODE = " 0123456789.ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,\\|!\"#$%&/()=?~*^+-{}[]€";
const char *constMEM alphaNumMask[1] MEMMODE = {alphaNum};
const char *constMEM digit MEMMODE = "0123456789";
const char *constMEM digitMask[] MEMMODE = {digit, digit, digit, ","};

MenuStatus menuStatus = IDLE;

menu_option_st<radio::RPT_MODE> rpt_mode_options[] = {{radio::repeaterNames[radio::RPT_MODE_OFF], radio::RPT_MODE_OFF},
                                                      {radio::repeaterNames[radio::RPT_MODE_POSITIVE], radio::RPT_MODE_POSITIVE},
                                                      {radio::repeaterNames[radio::RPT_MODE_NEGATIVE], radio::RPT_MODE_NEGATIVE}};

MODULATION_MODE modulation;
optionsPrompt<MODULATION_MODE> modulationMenu((const char *)"Modulation", modulation_options, modulation,
                                              sizeof(modulation_options) / sizeof(modulation_options[0]), [](MODULATION_MODE v) {
                                                  main_board::setModulationMode(v, true);
                                              });

optionsPrompt<radio::BAND> bandMenu((const char *)"Band", band_options, config.band, sizeof(band_options) / sizeof(band_options[0]), [](radio::BAND) {
    radio::set_band();
});

optionsPrompt<radio::BAND> filterMenu((const char *)"Frontend filter", band_options, config.filter, sizeof(band_options) / sizeof(band_options[0]),
                                      [](radio::BAND) {
                                          radio::filter = radio::BAND_NONE; // reset current applied filter
                                          main_board::set_filter();
                                      });

optionsPrompt<radio::IF_FILTER> IFFilterMenu((const char *)"IF filter", if_filter_options, config.if_filter,
                                             sizeof(if_filter_options) / sizeof(if_filter_options[0]), [](radio::IF_FILTER) {
                                                 if (!ISTX) {
                                                     // While transmitting, the filter is automatically set
                                                     main_board::set_if_filter(config.if_filter);
                                                     radio::update_freq();
                                                 }
                                             });

optionsPrompt<radio::RPT_MODE> repeaterMenu((const char *)"Repeater mode", rpt_mode_options, config.repeater_mode,
                                            sizeof(rpt_mode_options) / sizeof(rpt_mode_options[0]), [](radio::RPT_MODE) {
                                                radio::update_freq();
                                            });

void open_gain() {
    menu_exit();
    nav.doNav(navCmd(enterCmd));
    nav.doNav(navCmd(idxCmd, 0));
    nav.doNav(navCmd(idxCmd, 4));
}

menu_option_st<radio::FRONTEND_PATH> frontend_path_options[] = {
    {"Att. (-10 dB)", radio::FRONTEND_PATH_ATT}, {"Pass-thru (0 dB)", radio::FRONTEND_PATH_THRU}, {"LNA (20 dB)", radio::FRONTEND_PATH_LNA}

};

radio::FRONTEND_PATH frontend_path = config.frontend_path;

optionsPrompt<radio::FRONTEND_PATH> frontendPathMenu((const char *)"Frontend", frontend_path_options, frontend_path,
                                                     sizeof(frontend_path_options) / sizeof(frontend_path_options[0]), [](radio::FRONTEND_PATH) {
                                                         config.frontend_path = frontend_path;
                                                         main_board::update();
                                                     });

Menu::numberPrompt<float> squelchEditMenu((const char *)"Squelch", &config.squelch_level, 2, ' ', '.', nullptr,
                                          [](float) {
                                              sstrength::set_squelch(config.squelch_level);
                                          },
                                          0, 9);

void menu_exit() {
    stringIn<1> strIn;
    while (Menu::menuStatus == ACTIVE) {
        strIn.write('/'); // press esc multiple times to exit from whatever depth we're in
        nav.doInput(strIn);
    }
}

} // namespace Menu

using namespace Menu;

TOGGLE(config.squelch_auto, autoSquelch, "Squelch Auto: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, changeAutoSquelch, noEvent), VALUE("Off", false, changeAutoSquelch, noEvent));

TOGGLE(config.agc_enabled, enableAGCToggleMenu, "AGC: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("Enabled", true, changeAGCEnabled, noEvent), VALUE("Disabled", false, changeAGCEnabled, noEvent))

Menu::numberPrompt<uint32_t> repeaterOffsetMenu((const char *)"Repeater offset", &config.repeater_offset, 0, ' ', '.', "kHz", nullptr, 0, 100000, 500, 5000);

MENU(menuTune, "Tune", doNothing, anyEvent, noStyle, OBJ(modulationMenu), OBJ(bandMenu), OBJ(filterMenu), OBJ(IFFilterMenu), SUBMENU(enableAGCToggleMenu),
     SUBMENU(autoSquelch), OBJ(squelchEditMenu), OBJ(repeaterMenu), OBJ(repeaterOffsetMenu));

result changeHPAEnabled(eventMask) {
    main_board::update();
    return proceed;
}

result changeAutoSquelch(eventMask) {

    // Enable or disable squelch level setting
    if (config.squelch_auto) {
        menuTune[6].disable();
    } else {
        menuTune[6].enable();
    }

    return proceed;
}

result changeAGCEnabled(eventMask) {
    main_board::update();
    return proceed;
}

menu_option_st<LO_POWER> lo_power_options[] = {{"Low (-4 dBm)", LO_POWER_LOW}, {"Medium (0 dBm)", LO_POWER_MEDIUM}, {"High (4 dBm)", LO_POWER_HIGH}

};

optionsPrompt<LO_POWER> driveStrength1stLOMenu((const char *)"1st LO drive", lo_power_options, config.lo_drive_strength_0,
                                               sizeof(lo_power_options) / sizeof(lo_power_options[0]), [](LO_POWER) {
                                                   board::change_drive_strength = true;
                                               });

optionsPrompt<LO_POWER> driveStrength2ndLOMenu((const char *)"2nd LO drive", lo_power_options, config.lo_drive_strength_1,
                                               sizeof(lo_power_options) / sizeof(lo_power_options[0]), [](LO_POWER) {
                                                   board::change_drive_strength = true;
                                               });

menu_option_st<LO_INJECTION> lo_injection_options[] = {{"LO", LOW_SIDE}, {"HIGH", HIGH_SIDE}};

TOGGLE(config.hpa_enabled, enableHPAToggleMenu, "HPA: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("Enabled", true, changeHPAEnabled, noEvent), VALUE("Disabled", false, changeHPAEnabled, noEvent))

TOGGLE(config.debug, debugToggleMenu, "Debug: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent))

optionsPrompt<LO_INJECTION> loSideInjectionMenu((const char *)"Preferred LO inj. side", lo_injection_options, config.lo_injection,
                                                sizeof(lo_injection_options) / sizeof(lo_injection_options[0]), [](LO_INJECTION) {
                                                    board::change_drive_strength = true;
                                                });

#if ENABLE_RTC

RTC_TimeTypeDef time;
RTC_DateTypeDef date;

void setDate() {
    HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
    rtc_signal.emit(nullptr);
}

void setTime() {
    HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
    rtc_signal.emit(nullptr);
}

PADMENU(dateMenu, "Date", setDate, updateEvent, noStyle, FIELD(date.Year, "", "/", 22, 99, 1, 0, setDate, exitEvent, noStyle),
        FIELD(date.Month, "", "/", 1, 12, 1, 0, setDate, exitEvent, wrapStyle), FIELD(date.Date, "", "", 1, 31, 1, 0, setDate, exitEvent, wrapStyle));

PADMENU(timeMenu, "Time", setTime, updateEvent, noStyle, FIELD(time.Hours, "", ":", 0, 23, 1, 0, setTime, exitEvent, wrapStyle),
        FIELD(time.Minutes, "", "", 1, 59, 1, 0, setTime, exitEvent, wrapStyle));

#endif

Menu::numberPrompt<uint8_t> powerSavePeriod((const char *)"Power save period", &config.power_save_period_seconds, 0, ' ', '.', "s",
                                            [](uint16_t v) {
                                                standby::power_save(v);
                                            },
                                            0, 120, 10, 10);

Menu::numberPrompt<uint8_t> hpaPowerMenu((const char *)"Max HPA pow", &config.max_power_dbm, 0, ' ', '.', "dBm", nullptr, 0, 50, 1, 5);
Menu::numberPrompt<uint16_t> couplerOffsetMenu((const char *)"Coupler 0 dB offset", &config.coupler_0db_mv, 0, ' ', '.', "mV",
                                               [](uint16_t v) {
                                                   rf_coupler::set_offset(v);
                                               },
                                               0, 5000, 5, 100);
Menu::numberPrompt<int32_t> loRefCorrectionMenu((const char *)"LO Ref. Correction", &config.f_correction, 0, ' ', '.', "Hz",
                                                [](int32_t) {
                                                    board::change_calibration = true;
                                                },
                                                -1000000, 1000000, 1, 10);
Menu::numberPrompt<int32_t> ifCorrectionMenu((const char *)"IF Correction", &config.if_correction, 0, ' ', '.', "Hz",
                                             [](int32_t) {
                                                 board::change_calibration = true;
                                             },
                                             -1000000, 1000000, 1, 10);
Menu::numberPrompt<uint32_t> if1stFreqMenu((const char *)"1st. IF Frequency", &config.f_1st_if, 0, ' ', '.', "Hz",
                                           [](uint32_t) {
                                               board::change_calibration = true;
                                           },
                                           10000, 100000000, 1000, 10000);
Menu::numberPrompt<uint32_t> ifFMTXFreqMenu((const char *)"FM IF TX Frequency", &config.f_if_fm_tx, 0, ' ', '.', "Hz",
                                            [](uint32_t) {
                                                board::change_calibration = true;
                                            },
                                            10000, 100000000, 1000, 10000);

result settings_reset(eventMask) {
    config = Config();
    settings_write(&config);
    return proceed;
}

bool locked = false;
result set_usb_msc_mode(eventMask) {
    init_USB_MSC();
    return proceed;
}

void lock() {
    LockView view;
    view.paint();
    fft::fft_task.set_enabled(false);
    view_manager::task.set_enabled(false);
    locked = true;
}

void unlock() {
    fft::fft_task.set_enabled(true);
    view_manager::task.set_enabled(true);
    view_manager::mainView.set_dirty();
    locked = false;
}

MENU(menuSettings, "Settings", doNothing, anyEvent, noStyle, SUBMENU(debugToggleMenu), OBJ(powerSavePeriod), OP("Reset defaults", settings_reset, enterEvent),
     SUBMENU(enableHPAToggleMenu), OBJ(hpaPowerMenu), OBJ(Menu::frontendPathMenu), OBJ(couplerOffsetMenu), OBJ(driveStrength1stLOMenu),
     OBJ(driveStrength2ndLOMenu), OBJ(loSideInjectionMenu), OBJ(if1stFreqMenu), OBJ(ifFMTXFreqMenu), OBJ(loRefCorrectionMenu), OBJ(ifCorrectionMenu),
#if ENABLE_RTC
     SUBMENU(dateMenu), SUBMENU(timeMenu)
#endif
);

MENU(mainMenu, "Main menu", doNothing(), noEvent, noStyle, SUBMENU(menuTune),
#if DSP_ENABLED
     SUBMENU(dsp_ui::menuDSP),
#endif
     SUBMENU(scanner_ui::menuScan), SUBMENU(fftUI::fftMenu), SUBMENU(menuSettings), SUBMENU(boardUI::boardMenu), OBJ(freq_memory::freqMemMenu),
     OP("USB Mass Storage Device", set_usb_msc_mode, enterEvent));

const colorDef<uint16_t> menuColors[8] MEMMODE = {
    {{C565_TRANSPARENT, C565_TRANSPARENT}, {C565_BLACK, C565_TRANSPARENT, C565_TRANSPARENT}}, // bgColor
    {{C565_GREY_DARK, C565_GREY_DARK}, {C565_WHITE, C565_WHITE, C565_BLACK}},                 // fgColor
    {{C565_GREY_LIGHT, C565_GREY_LIGHT}, {C565_CYAN_DARK, C565_CYAN_DARK, C565_RED}},         // valColor
    {{C565_GREY_LIGHT, C565_GREY_LIGHT}, {C565_WHITE, C565_YELLOW, C565_YELLOW}},             // unitColor
    {{C565_TRANSPARENT, C565_BLACK}, {C565_TRANSPARENT, C565_GREY_DARK, C565_WHITE}},         // cursorColor
    {{C565_BLACK, C565_YELLOW}, {C565_BLUE, C565_RED, C565_RED}},                             // titleColor
    {{C565_BLACK, C565_YELLOW}, {C565_BLUE, C565_GREY_DARK, C565_BLUE}},                      // editBgColor
    {{C565_TRANSPARENT, C565_GREY_DARKER}, {C565_TRANSPARENT, C565_GREY_DARKER, C565_BLUE}}   // selectColor
};

#define MAX_DEPTH 5

Menu::idx_t tops[MAX_DEPTH];

uint16_t fontW = 7, fontH = 10;
short dispX = DISPLAY_X_PIXELS / fontW, dispY = INFO_HEIGHT / (fontH + 2);
panel panels[] MEMMODE = {{0, 0, dispX, dispY}};
navNode *panel_nodes[sizeof(panels) / sizeof(panel)];
panelsList panel_list(panels, panel_nodes, sizeof(panels) / sizeof(panel));

Menu::menuILI9431Out ili9431Out(lcd, menuColors, tops, panel_list, fontW, fontH + 2);
Menu::menuOut *const outs[] = {&ili9431Out};                // list of output devices
Menu::outputsList outList(const_cast<menuOut **>(outs), 1); // outputs list controller

// NULL input stream. We will control the menu programmatically
chainStream<0> in(NULL);

NAVROOT(nav, mainMenu, MAX_DEPTH, in, outList)

// when menu is suspended
result idle(menuOut &o, idleEvent e) {
    // o.clear();

    switch (e) {
        case idleStart:
            Menu::menuStatus = IDLE;
            view_manager::mainView.set_dirty();

            // Remove custom actions
            actions_signal.emit(nullptr);

            break;
        case idling:

            view_manager::mainView.set_dirty(); // Mark view as dirty to know we have to redraw all widgets next time
            break;
        case idleEnd:

            Menu::menuStatus = ACTIVE;

            // Add custom actions (they'll be captured by the bottom button bar)
            actions_signal.emit(&navigation_actions);
            break;
    }

    return proceed;
}

void menu_sdcard_callback(void *, void *args) {

    sdcard_st_info *info = (sdcard_st_info *)args;

    switch (info->status) {

        case MassStorageDeviceActive:
            lock();
            break;
        case Mounted:
            dspReplayUI::replayMenu.enable();
            dspCaptureUI::captureMenu.enable();

            if (locked) {
                unlock();
            }
            break;
        default:
            dspReplayUI::replayMenu.disable();
            dspCaptureUI::captureMenu.disable();
    }
}

void update_options() {
    dsp_ui::dsp_enabled = !ISANALOG;
    modulation = config.modulation;

    // Set enabled options for current mode
    for (auto &option : if_filter_options) {
        if (option.value != radio::IF_FILTER_AUTO) {
            option.enabled = radio::is_filter_allowed(option.value);
        }
    }

    dsp_ui::dsp_compressor_set();
}

void mode_signal_handler(void *, void *) {
    update_options();
}

void menu_setup() {

    nav.idleTask = idle; // point a function to be used when menu is suspended

    nav.idleOn(idle); // this menu will start on idle state, press select to enter menu

    nav.timeOut = 60;
    nav.useUpdateEvent = true;

    changeAutoSquelch(eventMask::noEvent);

#if ENABLE_SD_CARD
    sdcard_signal.add(NULL, menu_sdcard_callback);
    menu_sdcard_callback(NULL, &sdcard_info);
#endif

#if ENABLE_RTC
    HAL_RTC_GetTime(&hrtc, &time, FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, FORMAT_BIN);
#endif

    update_options();

    main_board::mode_signal.add(nullptr, mode_signal_handler);
}

void menu_size(int w, int h) {

    dispX = w / fontW, dispY = h / (fontH + 2);
    nav.out.outs[0]->panels.panels[0].h = dispY;
    nav.out.outs[0]->panels.panels[0].w = dispX;
    ((MenuWidget *)view_manager::mainView.Menu())->set_parent_rect({0, DISPLAY_Y_PIXELS - STATUS_HEIGHT - h, w, h});
}
