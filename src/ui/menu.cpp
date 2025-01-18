
#include "menu.h"
#include "Display_afb.h"
#include "config.h"
#include "lcd.h"
#include "main_view.h"
#include "../../lib/Menu/src/menuIO/chainStream.h"
#include "../../lib/Menu/src/menuIO/stringIn.h"
#include "../../lib/Menu/src/plugin/userMenu.h"
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
#include "status.h"
#include "settings.h"

using namespace Menu;

MenuStatus menuStatus = IDLE;

// result resetIQBalance(eventMask e, navNode &nav, prompt &item) {
//         printf("event: ");
//
//     printf("%d\n", e);
//     return proceed;
// }

result changeModulation(eventMask e) {
    main_board::setModulationMode(config.modulation, true);
    return proceed;
}

prompt *modulationValues[] = {new Menu::menuValue<MODULATION_MODE>(radio::modulationNames[AM], AM),
                              new Menu::menuValue<MODULATION_MODE>(radio::modulationNames[FM], FM),
                              new Menu::menuValue<MODULATION_MODE>(radio::modulationNames[WFM], WFM),
                              new Menu::menuValue<MODULATION_MODE>(radio::modulationNames[CW], CW),
                              new Menu::menuValue<MODULATION_MODE>(radio::modulationNames[SSB_LSB], SSB_LSB),
                              new Menu::menuValue<MODULATION_MODE>(radio::modulationNames[SSB_USB], SSB_USB)};

Menu::select<MODULATION_MODE> &modulationMenu = *new Menu::select<MODULATION_MODE>("Modulation", config.modulation, sizeof(modulationValues) / sizeof(prompt *),
                                                                                   modulationValues, changeModulation, exitEvent);

result changeBand(eventMask e) {
    radio::set_band();
    return proceed;
}

prompt *bandValues[] = {new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_AUTO], radio::BAND_AUTO),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_70cm], radio::BAND_70cm),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_1m], radio::BAND_1m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_2m], radio::BAND_2m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::AIRBAND], radio::AIRBAND),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_6m], radio::BAND_6m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_10m], radio::BAND_10m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_11m], radio::BAND_11m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_12m], radio::BAND_12m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_15m], radio::BAND_15m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_17m], radio::BAND_17m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_20m], radio::BAND_20m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_30m], radio::BAND_30m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_40m], radio::BAND_40m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_60m], radio::BAND_60m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_80m], radio::BAND_80m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_160m], radio::BAND_160m),
                        new Menu::menuValue<radio::BAND>(radio::bandNames[radio::BAND_ALL], radio::BAND_ALL)};

prompt *IFFilterValues[] = {
    new Menu::menuValue<radio::IF_FILTER>(radio::IFFilterNames[radio::IF_FILTER_AUTO], radio::IF_FILTER_AUTO),
    new Menu::menuValue<radio::IF_FILTER>(radio::IFFilterNames[radio::IF_FILTER_3KHZ], radio::IF_FILTER_3KHZ),
    new Menu::menuValue<radio::IF_FILTER>(radio::IFFilterNames[radio::IF_FILTER_15KHZ], radio::IF_FILTER_15KHZ),
    new Menu::menuValue<radio::IF_FILTER>(radio::IFFilterNames[radio::IF_FILTER_150KHZ], radio::IF_FILTER_150KHZ),
};

const char *colorNames[] = {"Black",        "Grey darker", "Grey dark", "Grey ligh", "White",  "Navy",     "Green dark", "Cyan dark",
                            "Maroon",       "Olive",       "Blue",      "Green",     "Red",    "Magenta",  "Yellow",     "Orange",
                            "Green-yellow", "Pink",        "Brown",     "Gold",      "Silver", "Sky blue", "Violet"};

prompt *colorValues[23] = {
    new Menu::menuValue<uint16_t>(colorNames[0], C565_BLACK),        new Menu::menuValue<uint16_t>(colorNames[1], C565_GREY_DARKER),
    new Menu::menuValue<uint16_t>(colorNames[2], C565_GREY_DARK),    new Menu::menuValue<uint16_t>(colorNames[3], C565_GREY_LIGHT),
    new Menu::menuValue<uint16_t>(colorNames[4], C565_WHITE),        new Menu::menuValue<uint16_t>(colorNames[5], C565_NAVY),
    new Menu::menuValue<uint16_t>(colorNames[6], C565_GREEN_DARK),   new Menu::menuValue<uint16_t>(colorNames[7], C565_CYAN_DARK),
    new Menu::menuValue<uint16_t>(colorNames[8], C565_MAROON),       new Menu::menuValue<uint16_t>(colorNames[9], C565_OLIVE),
    new Menu::menuValue<uint16_t>(colorNames[10], C565_BLUE),        new Menu::menuValue<uint16_t>(colorNames[11], C565_GREEN),
    new Menu::menuValue<uint16_t>(colorNames[12], C565_RED),         new Menu::menuValue<uint16_t>(colorNames[13], C565_MAGENTA),
    new Menu::menuValue<uint16_t>(colorNames[14], C565_YELLOW),      new Menu::menuValue<uint16_t>(colorNames[15], C565_ORANGE),
    new Menu::menuValue<uint16_t>(colorNames[16], C565_GREENYELLOW), new Menu::menuValue<uint16_t>(colorNames[17], C565_PINK),
    new Menu::menuValue<uint16_t>(colorNames[18], C565_BROWN),       new Menu::menuValue<uint16_t>(colorNames[19], C565_GOLD),
    new Menu::menuValue<uint16_t>(colorNames[20], C565_SILVER),      new Menu::menuValue<uint16_t>(colorNames[21], C565_SKYBLUE),
    new Menu::menuValue<uint16_t>(colorNames[22], C565_VIOLET),
};

prompt *repeaterValues[] = {
    new Menu::menuValue<radio::RPT_MODE>(radio::repeaterNames[radio::RPT_MODE_OFF], radio::RPT_MODE_OFF),
    new Menu::menuValue<radio::RPT_MODE>(radio::repeaterNames[radio::RPT_MODE_POSITIVE], radio::RPT_MODE_POSITIVE),
    new Menu::menuValue<radio::RPT_MODE>(radio::repeaterNames[radio::RPT_MODE_NEGATIVE], radio::RPT_MODE_NEGATIVE),
};

result changeRepeater(eventMask e) { // Update repeater mode
    radio::update_freq();
    return proceed;
}

Menu::select<radio::BAND> &bandMenu =
    *new Menu::select<radio::BAND>("Band", config.band, sizeof(bandValues) / sizeof(prompt *), bandValues, changeBand, exitEvent);

result changeFilter(eventMask e) { // synchronize current filter and config.filter, which is changed in the menu

    radio::filter = radio::BAND_NONE; // reset current applied filter
    main_board::set_filter();

    return proceed;
}

Menu::select<uint8_t> &filterMenu =
    *new Menu::select<uint8_t>("Frontend filter", config.filter, sizeof(bandValues) / sizeof(prompt *), bandValues, changeFilter, exitEvent);

result changeIFFilter(eventMask e) { // syncronize current filter and config.filter, which is changed in the menu

    if (!ISTX) {
        // While transmitting, the filter is automatically set
        main_board::set_if_filter(config.if_filter);
        radio::update_freq();
    }

    return proceed;
}

result set_squelch(eventMask e) {
    sstrength::set_squelch(config.squelch_level);
    return proceed;
}

Menu::select<radio::IF_FILTER> &IFFilterMenu =
    *new Menu::select<radio::IF_FILTER>("IF Filter", config.if_filter, sizeof(IFFilterValues) / sizeof(prompt *), IFFilterValues, changeIFFilter, exitEvent);

TOGGLE(config.squelch_auto, autoSquelch, "Squelch Auto: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, changeAutoSquelch, noEvent), VALUE("Off", false, changeAutoSquelch, noEvent));

TOGGLE(config.agc_enabled, enableAGCToggleMenu, "AGC: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("Enabled", true, changeAGCEnabled, noEvent), VALUE("Disabled", false, changeAGCEnabled, noEvent))

Menu::select<radio::RPT_MODE> &repeaterMenu = *new Menu::select<radio::RPT_MODE>(
    "Repeater mode", config.repeater_mode, sizeof(repeaterValues) / sizeof(prompt *), repeaterValues, changeRepeater, exitEvent);

MENU(menuTune, "Tune", doNothing, anyEvent, noStyle, SUBMENU(modulationMenu), SUBMENU(bandMenu), SUBMENU(filterMenu), SUBMENU(IFFilterMenu),
     SUBMENU(enableAGCToggleMenu), SUBMENU(autoSquelch), FIELD(config.squelch_level, "Squelch", "S", -0, 10, 1, 0.1, set_squelch, exitEvent, noStyle),
     SUBMENU(repeaterMenu),
     altFIELD(engPlaces<3>::menuField, config.repeater_offset, "Repeater offset: ", "kHz.", 0, 100000, 10000, 10000, changeRepeater, exitEvent, noStyle),
     EXIT("<Back"));

result changeHPAEnabled(eventMask e) {
    main_board::update();
    return proceed;
}

result changeAutoSquelch(eventMask e) {

    // Enable or disable squelch level setting
    if (config.squelch_auto) {
        menuTune[6].disable();
    } else {
        menuTune[6].enable();
    }

    return proceed;
}

result changeAGCEnabled(eventMask e) {
    main_board::update();
    return proceed;
}

prompt *driveStrengthValues[] = {new Menu::menuValue<LO_POWER>("Low (-4 dBm)", LO_POWER_LOW), new Menu::menuValue<LO_POWER>("Medium (0 dBm)", LO_POWER_MEDIUM),
                                 new Menu::menuValue<LO_POWER>("High (4 dBm)", LO_POWER_HIGH)

};

prompt *frontendPathValues[] = {new Menu::menuValue<radio::FRONTEND_PATH>("Attenuator (-10 dB)", radio::FRONTEND_PATH_ATT),
                                new Menu::menuValue<radio::FRONTEND_PATH>("Pass-thru (0 dB)", radio::FRONTEND_PATH_THRU),
                                new Menu::menuValue<radio::FRONTEND_PATH>("LNA (20 dB)", radio::FRONTEND_PATH_LNA)

};

prompt *loInjectionValues[] = {new Menu::menuValue<LO_INJECTION>("LO", LOW_SIDE), new Menu::menuValue<LO_INJECTION>("HIGH", HIGH_SIDE)};

TOGGLE(config.hpa_enabled, enableHPAToggleMenu, "HPA: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("Enabled", true, changeHPAEnabled, noEvent), VALUE("Disabled", false, changeHPAEnabled, noEvent))

TOGGLE(config.debug, debugToggleMenu, "Debug: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, doNothing, noEvent), VALUE("Off", false, doNothing, noEvent))

Menu::select<LO_INJECTION> &loSideInjectionMenu =
    *new Menu::select<LO_INJECTION>("Preferred LO inj. side", config.lo_injection, sizeof(loInjectionValues) / sizeof(prompt *), loInjectionValues);

result changeDriveStrength(eventMask e) {
    change_drive_strength = true;
    return proceed;
}

result changeCalibration(eventMask e) {
    change_calibration = true;
    return proceed;
}

result changeCouplerOffset(eventMask e) {
    rf_coupler::set_offset(config.coupler_0db_mv);
    return proceed;
}

radio::FRONTEND_PATH frontend_path = config.frontend_path;
result updateRadio(eventMask e) {
    config.frontend_path = frontend_path;
    main_board::update();
    return proceed;
}

Menu::select<LO_POWER> &driveStrength1stLOMenu = *new Menu::select<LO_POWER>(
    "1st LO drive", config.lo_drive_strength_0, sizeof(driveStrengthValues) / sizeof(prompt *), driveStrengthValues, changeDriveStrength, exitEvent);

Menu::select<LO_POWER> &driveStrength2ndLOMenu = *new Menu::select<LO_POWER>(
    "2nd LO drive", config.lo_drive_strength_1, sizeof(driveStrengthValues) / sizeof(prompt *), driveStrengthValues, changeDriveStrength, exitEvent);

Menu::select<radio::FRONTEND_PATH> &frontendPathMenu = *new Menu::select<radio::FRONTEND_PATH>(
    "Frontend", frontend_path, sizeof(frontendPathValues) / sizeof(prompt *), frontendPathValues, updateRadio, exitEvent);
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

MENU(menuSettings, "Settings", doNothing, anyEvent, noStyle, SUBMENU(debugToggleMenu), SUBMENU(enableHPAToggleMenu),
     FIELD(config.max_power_dbm, "HPA power limit", "dBm.", 0, 50, 1, 0, doNothing, exitEvent, wrapStyle), SUBMENU(frontendPathMenu),
     FIELD(config.coupler_0db_mv, "Coupler 0 dB offset", "mV.", 0, 5000, 5, 0, changeCouplerOffset, exitEvent, wrapStyle), SUBMENU(driveStrength1stLOMenu),
     SUBMENU(driveStrength2ndLOMenu), SUBMENU(loSideInjectionMenu),
     altFIELD(engPlaces<3>::menuField, config.f_1st_if, "1st. IF Frequency", "kHz.", 0, 100000000, 1000, 10000, changeCalibration, exitEvent, noStyle),
     altFIELD(engPlaces<3>::menuField, config.f_if_fm_tx, "FM IF TX Frequency", "kHz.", 0, 100000000, 1000, 10000, changeCalibration, exitEvent, noStyle),
     FIELD(config.f_correction, "LO Ref. Correction", "Hz.", -1000000, 1000000, 10, 1, changeCalibration, exitEvent, wrapStyle),
     FIELD(config.if_correction, "IF Correction", "Hz.", -1000000, 1000000, 10, 0, changeCalibration, exitEvent, wrapStyle),
#if ENABLE_RTC
     SUBMENU(dateMenu), SUBMENU(timeMenu),
#endif
     EXIT("<Back"));

/*
 ****************** DSP MENU *****************
 */

#if DSP_ENABLED
/* TODO: Disable SD card related functionality if card is not enabled */
MENU(menuDSP, "DSP", doNothing, anyEvent, noStyle, SUBMENU(dspCaptureUI::captureMenu), SUBMENU(dspReplayUI::replayMenu),
     SUBMENU(dspSignalGeneratorUI::signalGeneratorMenu), EXIT("<Back"));

#endif

/*
 * ***************** MEMORY MENU *******************
 */

// st_freq_mem temporary register
st_freq_mem tempFreqMem;
char tempFreqBuf[] = "000,000,000";

// Character validators for the frequency memories
const char *constMEM alphaNum MEMMODE = " 0123456789.ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,\\|!\"#$%&/()=?~*^+-{}[]€";
const char *constMEM alphaNumMask[1] MEMMODE = {alphaNum};
const char *constMEM digit MEMMODE = "0123456789";
const char *constMEM digitMask[] MEMMODE = {digit, digit, digit, ","};

// A function to save the edited data record
result saveTarget(eventMask e, navNode &nav) {
    trace(MENU_DEBUG_OUT << "saveTarget" << endl);
    navNode &nn = nav.root->path[nav.root->level - 1];
    idx_t n = nn.sel; // get selection of previous level
    char *ptr;
    removePunct(tempFreqBuf);
    tempFreqMem.freq = strtol(tempFreqBuf, &ptr, 10);
    config.freqs[n] = tempFreqMem;

    using namespace status;
    if (settings_write(&config) == HAL_FLASH_ERROR_NONE) {
        handleError(ST_INFO, "Configuration saved");
    } else {
        handleError(ST_ERROR, "Error saving configuration");
    }
    return quit;
}

class labelPrompt : public prompt {
  public:
    char *value;

    labelPrompt(const char *text, char *value, action a = doNothing, eventMask e = noEvent, styles s = noStyle,
                systemStyles ss = ((Menu::systemStyles)(Menu::_parentDraw)))
        : prompt(text, a, e, s, ss), value(value) {}
    Used printTo(navRoot &root, bool sel, menuOut &out, idx_t idx, idx_t len, idx_t) override {
        len -= out.printRaw(shadow->text, len);
        len -= out.printRaw(": ", len);
        out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
        len -= out.printRaw(value, len);

        return len;
    }
};

result edit_freq_name(eventMask e, navNode &nav) {

    view_manager::keyboardView.set_text(tempFreqMem.name);
    view_manager::keyboardView.set_label("Name");
    view_manager::keyboardView.set_size(FREQ_MEM_NAME_SIZE);
    view_manager::keyboardView.on_changed = [](char *str) { strncpy(tempFreqMem.name, str, FREQ_MEM_NAME_SIZE); };
    view_manager::push(&view_manager::keyboardView);
    return proceed;
}

result edit_freq(eventMask e, navNode &nav) {
    view_manager::keypadView.set_value(tempFreqMem.freq, 0, "Hz", "Frequency");
    view_manager::keypadView.on_changed = [](double v) {
        tempFreqMem.freq = v;
        char buf[16];
        format_long(tempFreqMem.freq, buf);
        sprintf(tempFreqBuf, "%s", buf);
    };
    view_manager::push(&view_manager::keypadView);
    return proceed;
}

Menu::select<MODULATION_MODE> &freqMemModulationMenu =
    *new Menu::select<MODULATION_MODE>("Modulation", tempFreqMem.mode, sizeof(modulationValues) / sizeof(prompt *), modulationValues, updateRadio, exitEvent);

labelPrompt freqNameMenu((const char *)"Name", tempFreqMem.name, edit_freq_name, enterEvent, noStyle);
labelPrompt freqEditMenu((const char *)"Frequency", tempFreqBuf, edit_freq, enterEvent, noStyle);

// If you want to print the data record name as the title,
// then you MUST create a customized print menu to replace this default one
MENU(freqMemEditMenu, "Frequency edit", doNothing, noEvent, wrapStyle, OBJ(freqNameMenu), SUBMENU(freqMemModulationMenu), OBJ(freqEditMenu),
     OP("Save", saveTarget, enterEvent), EXIT("<Back"));

// Custom frequency memory menu
struct FreqMemoryMenu : UserMenu {
    using UserMenu::UserMenu;

    // Override sz() function to have variable/custom size
    // If using exit option an extra element has to be considered...
    // inline idx_t sz() const override {return 0;}

    Used printItem(menuOut &out, int idx, int len) override {

        if (len) {
            char buf[35], sf[10];
            bool empty;
            st_freq_mem fm = config.freqs[idx];
            empty = fm.freq == 0;
            if (empty) {
                sprintf(buf, "[%2d]", idx);
            } else {
                format_long(fm.freq, sf);
                snprintf(buf, 35, "[%2d] %-4s %12s  %*s", idx, radio::modulationNames[fm.mode], sf, FREQ_MEM_NAME_SIZE, fm.name);
            }

            return out.printText(buf, 35);
        } else {
            return 0;
        }
    }

    void doNav(Menu::navNode &nav, Menu::navCmd cmd) override {

        switch (cmd.cmd) {
            case idxCmd: // Index selected

                if (config.freqs[nav.sel].freq) {
                    radio::set_frequency(config.freqs[nav.sel].freq);
                }

                // Exit menu
                // UserMenu::doNav(nav, Menu::escCmd);
                break;

            default:
                UserMenu::doNav(nav, cmd);
                break;
        }
    }
};

result freqMemorySelectedEvent(eventMask e, navNode &nav);

FreqMemoryMenu freqMemMenu("Frequency memory", FREQ_MEM_SIZE, "<Back", freqMemEditMenu, freqMemorySelectedEvent, enterEvent);

/*
 * This will be called whenever an entry is selected in the frequency memory
 * It copies the currently selected index st_freq_mem in the temporary struct
 */
result freqMemorySelectedEvent(eventMask e, navNode &nav) {
    trace(MENU_DEBUG_OUT << "copy data to temp target:" << (int)nav.target << "\n");
    if (nav.target == &freqMemMenu) { // Only if we are on memory menu
        tempFreqMem = config.freqs[nav.sel];

        // If it's empty: New entry. Use current frequency
        if (!tempFreqMem.freq) {
            tempFreqMem.freq = radio::get_frequency();
            tempFreqMem.mode = config.modulation;
        }

        char buf[16];
        format_long(tempFreqMem.freq, buf);
        sprintf(tempFreqBuf, "%s", buf);
    }
    // nav.sel can be stored for future reference
    return proceed;
}

/*
 * ****************** END MEMORY MENU *******************
 */

MENU(mainMenu, "Main menu", doNothing, noEvent, noStyle, SUBMENU(menuTune),
#if DSP_ENABLED
     SUBMENU(menuDSP),
#endif
     SUBMENU(scanner_ui::menuScan), SUBMENU(fftUI::fftMenu), SUBMENU(menuSettings), SUBMENU(boardUI::boardMenu), OBJ(freqMemMenu), EXIT("<Back"));

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
            menuStatus = IDLE;
            view_manager::mainView.set_dirty();
            // o.println("suspending menu!");
            break;
        case idling:
            // o.println("suspended...");
            view_manager::mainView.set_dirty(); // Mark view as dirty to know we have to redraw all widgets next time
            break;
        case idleEnd:
            // o.println("resuming menu.");
            menuStatus = ACTIVE;

            break;
    }

    return proceed;
}

void menu_sdcard_callback(void *thisptr, void *args) {

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
    while (menuStatus == ACTIVE) {
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
