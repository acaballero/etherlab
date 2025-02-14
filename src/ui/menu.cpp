
#include "menu.h"
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
#include "ui/scanner_ui.h"
#include "dsp/fft/fft_ui.h"
#include "main.h"
#include "main_board.h"
#include "fatfs/fatfs.h"
#include "radio.h"
#include "rf_coupler.h"
#include "../../lib/utils/utils.hpp"
#include "view_manager.h"
#include "s_strength.h"
#include <cstring>
#include "settings.h"
#include "menu_options.h"
#include "menu_prompts.h"
#include "frequency_memory_ui.hpp"

namespace Menu {

MenuStatus menuStatus = IDLE;

menu_option_st<radio::RPT_MODE> rpt_mode_options[] = {{radio::repeaterNames[radio::RPT_MODE_OFF], radio::RPT_MODE_OFF},
                                                      {radio::repeaterNames[radio::RPT_MODE_POSITIVE], radio::RPT_MODE_POSITIVE},
                                                      {radio::repeaterNames[radio::RPT_MODE_NEGATIVE], radio::RPT_MODE_NEGATIVE}};

optionsPrompt<MODULATION_MODE> modulationMenu((const char *)"Modulation", modulation_options, config.modulation,
                                              sizeof(modulation_options) / sizeof(modulation_options[0]),
                                              [](MODULATION_MODE v) { main_board::setModulationMode(v, true); });

optionsPrompt<radio::BAND> bandMenu((const char *)"Band", band_options, config.band, sizeof(band_options) / sizeof(band_options[0]),
                                    [](radio::BAND) { radio::set_band(); });

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
                                            sizeof(rpt_mode_options) / sizeof(rpt_mode_options[0]), [](radio::RPT_MODE) { radio::update_freq(); });

} // namespace Menu

using namespace Menu;

TOGGLE(config.squelch_auto, autoSquelch, "Squelch Auto: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, changeAutoSquelch, noEvent), VALUE("Off", false, changeAutoSquelch, noEvent));

TOGGLE(config.agc_enabled, enableAGCToggleMenu, "AGC: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("Enabled", true, changeAGCEnabled, noEvent), VALUE("Disabled", false, changeAGCEnabled, noEvent))

Menu::numberPrompt<float> squelchEditMenu((const char *)"Squelch", &config.squelch_level, 2, ' ', '.', nullptr,
                                          [](float) { sstrength::set_squelch(config.squelch_level); }, 0, 9);

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

menu_option_st<radio::FRONTEND_PATH> frontend_path_options[] = {
    {"Attenuator (-10 dB)", radio::FRONTEND_PATH_ATT}, {"Pass-thru (0 dB)", radio::FRONTEND_PATH_THRU}, {"LNA (20 dB)", radio::FRONTEND_PATH_LNA}

};

optionsPrompt<LO_POWER> driveStrength1stLOMenu((const char *)"1st LO drive", lo_power_options, config.lo_drive_strength_0,
                                               sizeof(lo_power_options) / sizeof(lo_power_options[0]), [](LO_POWER) { board::change_drive_strength = true; });

optionsPrompt<LO_POWER> driveStrength2ndLOMenu((const char *)"2nd LO drive", lo_power_options, config.lo_drive_strength_1,
                                               sizeof(lo_power_options) / sizeof(lo_power_options[0]), [](LO_POWER) { board::change_drive_strength = true; });

radio::FRONTEND_PATH frontend_path = config.frontend_path;
optionsPrompt<radio::FRONTEND_PATH> frontendPathMenu((const char *)"Frontend", frontend_path_options, frontend_path,
                                                     sizeof(frontend_path_options) / sizeof(frontend_path_options[0]), [](radio::FRONTEND_PATH) {
                                                         config.frontend_path = frontend_path;
                                                         main_board::update();
                                                     });

menu_option_st<LO_INJECTION> lo_injection_options[] = {{"LO", LOW_SIDE}, {"HIGH", HIGH_SIDE}};

TOGGLE(config.hpa_enabled, enableHPAToggleMenu, "HPA: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("Enabled", true, changeHPAEnabled, noEvent), VALUE("Disabled", false, changeHPAEnabled, noEvent))

TOGGLE(config.debug, debugToggleMenu, "Debug: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent))

optionsPrompt<LO_INJECTION> loSideInjectionMenu((const char *)"Preferred LO inj. side", lo_injection_options, config.lo_injection,
                                                sizeof(lo_injection_options) / sizeof(lo_injection_options[0]),
                                                [](LO_INJECTION) { board::change_drive_strength = true; });

#if ENABLE_RTC

RTC_TimeTypeDef time;
RTC_DateTypeDef date;

void setDate() { HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN); }

void setTime() { HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN); }

PADMENU(dateMenu, "Date", setDate, updateEvent, noStyle, FIELD(date.Year, "", "/", 22, 99, 1, 0, setDate, exitEvent, noStyle),
        FIELD(date.Month, "", "/", 1, 12, 1, 0, setDate, exitEvent, wrapStyle), FIELD(date.Date, "", "", 1, 31, 1, 0, setDate, exitEvent, wrapStyle));

PADMENU(timeMenu, "Time", setTime, updateEvent, noStyle, FIELD(time.Hours, "", ":", 0, 23, 1, 0, setTime, exitEvent, wrapStyle),
        FIELD(time.Minutes, "", "", 1, 59, 1, 0, setTime, exitEvent, wrapStyle));

#endif

Menu::numberPrompt<uint8_t> hpaPowerMenu((const char *)"Max HPA pow", &config.max_power_dbm, 0, ' ', '.', "dBm", nullptr, 0, 50, 1, 5);
Menu::numberPrompt<uint16_t> couplerOffsetMenu((const char *)"Coupler 0 dB offset", &config.coupler_0db_mv, 0, ' ', '.', "mV",
                                               [](uint16_t v) { rf_coupler::set_offset(v); }, 0, 5000, 5, 100);
Menu::numberPrompt<int32_t> loRefCorrectionMenu((const char *)"LO Ref. Correction", &config.f_correction, 0, ' ', '.', "Hz",
                                                [](int32_t) { board::change_calibration = true; }, -1000000, 1000000, 1, 10);
Menu::numberPrompt<int32_t> ifCorrectionMenu((const char *)"IF Correction", &config.if_correction, 0, ' ', '.', "Hz",
                                             [](int32_t) { board::change_calibration = true; }, -1000000, 1000000, 1, 10);
Menu::numberPrompt<uint32_t> if1stFreqMenu((const char *)"1st. IF Frequency", &config.f_1st_if, 0, ' ', '.', "Hz",
                                           [](uint32_t) { board::change_calibration = true; }, 10000, 100000000, 1000, 10000);
Menu::numberPrompt<uint32_t> ifFMTXFreqMenu((const char *)"FM IF TX Frequency", &config.f_if_fm_tx, 0, ' ', '.', "Hz",
                                            [](uint32_t) { board::change_calibration = true; }, 10000, 100000000, 1000, 10000);

MENU(menuSettings, "Settings", doNothing, anyEvent, noStyle, SUBMENU(debugToggleMenu), SUBMENU(enableHPAToggleMenu), OBJ(hpaPowerMenu), OBJ(frontendPathMenu),
     OBJ(couplerOffsetMenu), OBJ(driveStrength1stLOMenu), OBJ(driveStrength2ndLOMenu), OBJ(loSideInjectionMenu), OBJ(if1stFreqMenu), OBJ(ifFMTXFreqMenu),
     OBJ(loRefCorrectionMenu), OBJ(ifCorrectionMenu),
#if ENABLE_RTC
     SUBMENU(dateMenu), SUBMENU(timeMenu)
#endif
);

/*
 ****************** DSP MENU *****************
 */

#if DSP_ENABLED
/* TODO: Disable SD card related functionality if card is not enabled */
MENU(menuDSP, "DSP", doNothing, anyEvent, noStyle, SUBMENU(dspCaptureUI::captureMenu), SUBMENU(dspReplayUI::replayMenu),
     SUBMENU(dspSignalGeneratorUI::signalGeneratorMenu));

#endif

MENU(mainMenu, "Main menu", doNothing, noEvent, noStyle, SUBMENU(menuTune),
#if DSP_ENABLED
     SUBMENU(menuDSP),
#endif
     SUBMENU(scanner_ui::menuScan), SUBMENU(fftUI::fftMenu), SUBMENU(menuSettings), SUBMENU(boardUI::boardMenu), OBJ(freqMemMenu));

const colorDef<uint16_t> menuColors[8] MEMMODE = {
    {{C565_TRANSPARENT, C565_TRANSPARENT}, {C565_BLACK, C565_TRANSPARENT, C565_TRANSPARENT}}, // bgColor
    {{C565_GREY_DARK, C565_GREY_DARK}, {C565_WHITE, C565_WHITE, C565_BLACK}},                 // fgColor
    {{C565_GREY_LIGHT, C565_GREY_LIGHT}, {C565_YELLOW, C565_YELLOW, C565_RED}},               // valColor
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
            // o.println("suspending menu!");
            break;
        case idling:
            // o.println("suspended...");
            view_manager::mainView.set_dirty(); // Mark view as dirty to know we have to redraw all widgets next time
            break;
        case idleEnd:
            // o.println("resuming menu.");
            Menu::menuStatus = ACTIVE;

            break;
    }

    return proceed;
}

void menu_sdcard_callback(void *, void *args) {

    sdcard_st_info *info = (sdcard_st_info *)args;

    if (info->status == Mounted) {
        dspReplayUI::replayMenu.enable();
        dspCaptureUI::captureMenu.enable();
    } else {
        dspReplayUI::replayMenu.disable();
        dspCaptureUI::captureMenu.disable();
    }
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
}

void menu_exit() {
    stringIn<1> strIn;
    while (Menu::menuStatus == ACTIVE) {
        strIn.write('/'); // press esc multiple times to exit from whatever depth we're in
        nav.doInput(strIn);
    }
}

void menu_size(int w, int h) {

    dispX = w / fontW, dispY = h / (fontH + 2);
    nav.out.outs[0]->panels.panels[0].h = dispY;
    nav.out.outs[0]->panels.panels[0].w = dispX;
    ((MenuWidget *)view_manager::mainView.Menu())->set_parent_rect({0, MENU_START_Y, w, h});
}
