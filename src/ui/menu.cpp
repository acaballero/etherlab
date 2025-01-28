
#include "menu.h"
#include "Display_afb.h"
#include "config.h"
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
#include <sys/_stdint.h>
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

/***************************/
//   START NEW MENU DEFINITIONS (progressive replacement from ArduinoMenu library)
/***************************/

menu_option_st<MODULATION_MODE> modulation_options[] = {{radio::modulationNames[AM], AM},           {radio::modulationNames[FM], FM},
                                                        {radio::modulationNames[WFM], WFM},         {radio::modulationNames[CW], CW},
                                                        {radio::modulationNames[SSB_LSB], SSB_LSB}, {radio::modulationNames[SSB_USB], SSB_USB}};

menu_option_st<radio::BAND> band_options[] = {{radio::bandNames[radio::BAND_AUTO], radio::BAND_AUTO}, {radio::bandNames[radio::BAND_70cm], radio::BAND_70cm},
                                              {radio::bandNames[radio::BAND_1m], radio::BAND_1m},     {radio::bandNames[radio::BAND_2m], radio::BAND_2m},
                                              {radio::bandNames[radio::AIRBAND], radio::AIRBAND},     {radio::bandNames[radio::BAND_6m], radio::BAND_6m},
                                              {radio::bandNames[radio::BAND_10m], radio::BAND_10m},   {radio::bandNames[radio::BAND_11m], radio::BAND_11m},
                                              {radio::bandNames[radio::BAND_12m], radio::BAND_12m},   {radio::bandNames[radio::BAND_15m], radio::BAND_15m},
                                              {radio::bandNames[radio::BAND_17m], radio::BAND_17m},   {radio::bandNames[radio::BAND_20m], radio::BAND_20m},
                                              {radio::bandNames[radio::BAND_30m], radio::BAND_30m},   {radio::bandNames[radio::BAND_40m], radio::BAND_40m},
                                              {radio::bandNames[radio::BAND_60m], radio::BAND_60m},   {radio::bandNames[radio::BAND_80m], radio::BAND_80m},
                                              {radio::bandNames[radio::BAND_160m], radio::BAND_160m}, {radio::bandNames[radio::BAND_ALL], radio::BAND_ALL}};

menu_option_st<radio::IF_FILTER> if_filter_options[] = {
    {radio::IFFilterNames[radio::IF_FILTER_AUTO], radio::IF_FILTER_AUTO},
    {radio::IFFilterNames[radio::IF_FILTER_3KHZ], radio::IF_FILTER_3KHZ},
    {radio::IFFilterNames[radio::IF_FILTER_15KHZ], radio::IF_FILTER_15KHZ},
    {radio::IFFilterNames[radio::IF_FILTER_150KHZ], radio::IF_FILTER_150KHZ},
};

const char *colorNames[] = {"Black",        "Grey darker", "Grey dark", "Grey ligh", "White",  "Navy",     "Green dark", "Cyan dark",
                            "Maroon",       "Olive",       "Blue",      "Green",     "Red",    "Magenta",  "Yellow",     "Orange",
                            "Green-yellow", "Pink",        "Brown",     "Gold",      "Silver", "Sky blue", "Violet"};

namespace Menu {
template class optionsPrompt<uint16_t>;

menu_option_st<uint16_t> color_options[] = {
    {"   ", C565_BLACK, C565_BLACK, C565_BLACK},
    {"   ", C565_GREY_DARKER, C565_GREY_DARKER, C565_GREY_DARKER},
    {"   ", C565_GREY_DARK, C565_GREY_DARK, C565_GREY_DARK},
    {"   ", C565_GREY_LIGHT, C565_GREY_LIGHT, C565_GREY_LIGHT},
    {"   ", C565_WHITE, C565_WHITE, C565_WHITE},
    {"   ", C565_NAVY, C565_NAVY, C565_NAVY},
    {"   ", C565_GREEN_DARK, C565_GREEN_DARK, C565_GREEN_DARK},
    {"   ", C565_CYAN_DARK, C565_CYAN_DARK, C565_CYAN_DARK},
    {"   ", C565_MAROON, C565_MAROON, C565_MAROON},
    {"   ", C565_OLIVE, C565_OLIVE, C565_OLIVE},
    {"   ", C565_BLUE, C565_BLUE, C565_BLUE},
    {"   ", C565_GREEN, C565_GREEN, C565_GREEN},
    {"   ", C565_RED, C565_RED, C565_RED},
    {"   ", C565_MAGENTA, C565_MAGENTA, C565_MAGENTA},
    {"   ", C565_YELLOW, C565_YELLOW, C565_YELLOW},
    {"   ", C565_ORANGE, C565_ORANGE, C565_ORANGE},
    {"   ", C565_GREENYELLOW, C565_GREENYELLOW, C565_GREENYELLOW},
    {"   ", C565_PINK, C565_PINK, C565_PINK},
    {"   ", C565_BROWN, C565_BROWN, C565_BROWN},
    {"   ", C565_GOLD, C565_GOLD, C565_GOLD},
    {"   ", C565_SILVER, C565_SILVER, C565_SILVER},
    {"   ", C565_SKYBLUE, C565_SKYBLUE, C565_SKYBLUE},
    {"   ", C565_VIOLET, C565_VIOLET, C565_VIOLET},
};

} // namespace Menu

menu_option_st<radio::RPT_MODE> rpt_mode_options[] = {{radio::repeaterNames[radio::RPT_MODE_OFF], radio::RPT_MODE_OFF},
                                                      {radio::repeaterNames[radio::RPT_MODE_POSITIVE], radio::RPT_MODE_POSITIVE},
                                                      {radio::repeaterNames[radio::RPT_MODE_NEGATIVE], radio::RPT_MODE_NEGATIVE}};

template <typename T> void open_option_buttons(menu_options_t<T> options, const char *title, T &value, uint16_t size, std::function<void(T)> on_select) {

    view_manager::optionButtonsView.clear();

    const auto fn = [&value, options, on_select](uint16_t index) {
        view_manager::optionButtonsView.set_visible(false);

        value = options[index].value;
        if (on_select) {
            on_select(value);
        }
    };

    for (int i = 0; i < size; i++) {
        menu_option_st<T> option = options[i];

        view_manager::optionButtonsView.add_item(option.name, nullptr, value == option.value, option.fg_color, option.bg_color);
        view_manager::optionButtonsView.on_select = fn;
    }
    view_manager::optionButtonsView.set_title(title);
    view_manager::push(&view_manager::optionButtonsView);
}

template <typename T>
optionsPrompt<T>::optionsPrompt(const char *text, menu_options_t<T> options, T &value, size_t size, std::function<void(T)> on_select, eventMask e, styles s,
                                systemStyles ss)
    : prompt(text, static_cast<action>([](Menu::eventMask, Menu::navNode &, Menu::prompt &item) {
                 optionsPrompt<T> prompt = static_cast<optionsPrompt<T> &>(item);
                 open_option_buttons<T>(prompt.options, item.getText(), prompt.value, prompt.size, [prompt](T m) { prompt.on_select(m); });
                 return proceed;
             }),
             e, s, ss),
      value(value), options(options), size(size), on_select(on_select) {}

/************************** */
//   END NEW MENU DEFINITIOS
/***************************/

optionsPrompt<MODULATION_MODE> modulationMenu((const char *)"Modulation", modulation_options, config.modulation,
                                              sizeof(modulation_options) / sizeof(modulation_options[0]),
                                              [](MODULATION_MODE v) { main_board::setModulationMode(v, true); });

optionsPrompt<radio::BAND> bandMenu((const char *)"Band", band_options, config.band, sizeof(band_options) / sizeof(band_options[0]),
                                    [](radio::BAND) { radio::set_band(); });

result changeRepeater(eventMask) { // Update repeater mode

    return proceed;
}

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

result set_squelch(eventMask) {
    sstrength::set_squelch(config.squelch_level);
    return proceed;
}

TOGGLE(config.squelch_auto, autoSquelch, "Squelch Auto: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("On", true, changeAutoSquelch, noEvent), VALUE("Off", false, changeAutoSquelch, noEvent));

TOGGLE(config.agc_enabled, enableAGCToggleMenu, "AGC: ", doNothing, noEvent, noStyle //,doExit,enterEvent,noStyle
       ,
       VALUE("Enabled", true, changeAGCEnabled, noEvent), VALUE("Disabled", false, changeAGCEnabled, noEvent))

MENU(menuTune, "Tune", doNothing, anyEvent, noStyle, OBJ(modulationMenu), OBJ(bandMenu), OBJ(filterMenu), OBJ(IFFilterMenu), SUBMENU(enableAGCToggleMenu),
     SUBMENU(autoSquelch), FIELD(config.squelch_level, "Squelch", "S", -0, 10, 1, 0.1, set_squelch, exitEvent, noStyle), OBJ(repeaterMenu),
     altFIELD(engPlaces<3>::menuField, config.repeater_offset, "Repeater offset: ", "kHz.", 0, 100000, 10000, 10000, changeRepeater, exitEvent, noStyle),
     EXIT("<Back"));

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
                                               sizeof(lo_power_options) / sizeof(lo_power_options[0]), [](LO_POWER) { change_drive_strength = true; });

optionsPrompt<LO_POWER> driveStrength2ndLOMenu((const char *)"2nd LO drive", lo_power_options, config.lo_drive_strength_1,
                                               sizeof(lo_power_options) / sizeof(lo_power_options[0]), [](LO_POWER) { change_drive_strength = true; });

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
                                                [](LO_INJECTION) { change_drive_strength = true; });

result changeCalibration(eventMask) {
    change_calibration = true;
    return proceed;
}

result changeCouplerOffset(eventMask) {
    rf_coupler::set_offset(config.coupler_0db_mv);
    return proceed;
}

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
     FIELD(config.max_power_dbm, "HPA power limit", "dBm.", 0, 50, 1, 0, doNothing, exitEvent, wrapStyle), OBJ(frontendPathMenu),
     FIELD(config.coupler_0db_mv, "Coupler 0 dB offset", "mV.", 0, 5000, 5, 0, changeCouplerOffset, exitEvent, wrapStyle), OBJ(driveStrength1stLOMenu),
     OBJ(driveStrength2ndLOMenu), OBJ(loSideInjectionMenu),
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
result saveTarget(eventMask, navNode &nav) {
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

// Explicit template instantiations for specific types
template Used numberPrompt<double>::printTo(navRoot &, bool, menuOut &, idx_t, idx_t, idx_t);
template Used numberPrompt<uint64_t>::printTo(navRoot &, bool, menuOut &, idx_t, idx_t, idx_t);

template <typename T> idx_t numberPrompt<T>::printTo(navRoot &, bool sel, menuOut &out, idx_t, idx_t len, idx_t) {
    len -= out.printRaw(shadow->text, len);
    len -= out.printRaw(": ", len);
    out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
    char buf[20];

    if (std::is_same<T, double>::value) {
        sprintf(buf, "%f", (double)*value);
    } else {
        format_long((int64_t)*value, buf, 0, thow_separator);
    }

    len -= out.printRaw(buf, len);

    out.setColor(Menu::unitColor, sel, Menu::enabledStatus, false);
    len -= out.printRaw(" ", len);
    len -= out.printRaw(unit, len);
    return len;
}

template result numberPrompt<double>::eventHandler(eventMask, navNode &, idx_t);
template result numberPrompt<uint64_t>::eventHandler(eventMask, navNode &, idx_t);

template <typename T> result numberPrompt<T>::eventHandler(eventMask e, navNode &, idx_t) {

    if (e == Menu::enterEvent) {
        view_manager::keypadView.set_value(*value, 0, unit, shadow->text);
        view_manager::keypadView.on_changed = [this](double v) { *value = v; };
        view_manager::push(&view_manager::keypadView);
    }

    return proceed;
}

result edit_freq_name(eventMask, navNode &) {

    view_manager::keyboardView.set_text(tempFreqMem.name);
    view_manager::keyboardView.set_label("Name");
    view_manager::keyboardView.set_size(FREQ_MEM_NAME_SIZE);
    view_manager::keyboardView.on_changed = [](char *str) { strncpy(tempFreqMem.name, str, FREQ_MEM_NAME_SIZE); };
    view_manager::push(&view_manager::keyboardView);
    return proceed;
}

result edit_freq(eventMask, navNode &) {
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

// Menu::select<MODULATION_MODE> &freqMemModulationMenu =
//     *new Menu::select<MODULATION_MODE>("Modulation", tempFreqMem.mode, sizeof(modulationValues) / sizeof(prompt *), modulationValues, updateRadio,
//     exitEvent);

labelPrompt freqNameMenu((const char *)"Name", tempFreqMem.name, edit_freq_name, enterEvent, noStyle);
labelPrompt freqEditMenu((const char *)"Frequency", tempFreqBuf, edit_freq, enterEvent, noStyle);

// If you want to print the data record name as the title,
// then you MUST create a customized print menu to replace this default one
MENU(freqMemEditMenu, "Frequency edit", doNothing, noEvent, wrapStyle, OBJ(freqNameMenu), OBJ(modulationMenu), OBJ(freqEditMenu),
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
