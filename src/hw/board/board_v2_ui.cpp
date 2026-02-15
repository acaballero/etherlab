//
// Created by Angel Dust on 16/04/2021.
//

#include "board_v2_ui.h"
#include "board_v2.h"
#include "../../settings.h"
#include "../../ui/main_view.h"

using namespace Menu;

namespace boardUI {

const char *IFGainNames[] = {"0 dB", "-6 dB", "-12 dB", "-18 dB", "-24 dB", "-30 db"};

result changeCMX973Gain(eventMask e, navNode &nav, Menu::prompt &item);

prompt *CMX973_VGB_Gains[] = {new Menu::menuValue<IF_GAIN>(IFGainNames[IF_GAIN_0], IF_GAIN_0, changeCMX973Gain, focusEvent),
                              new Menu::menuValue<IF_GAIN>(IFGainNames[IF_GAIN_MINUS6], IF_GAIN_MINUS6, changeCMX973Gain, focusEvent),
                              new Menu::menuValue<IF_GAIN>(IFGainNames[IF_GAIN_MINUS12], IF_GAIN_MINUS12, changeCMX973Gain, focusEvent),
                              new Menu::menuValue<IF_GAIN>(IFGainNames[IF_GAIN_MINUS18], IF_GAIN_MINUS18, changeCMX973Gain, focusEvent),
                              new Menu::menuValue<IF_GAIN>(IFGainNames[IF_GAIN_MINUS24], IF_GAIN_MINUS24, changeCMX973Gain, focusEvent),
                              new Menu::menuValue<IF_GAIN>(IFGainNames[IF_GAIN_MINUS30], IF_GAIN_MINUS30, changeCMX973Gain, focusEvent)};

prompt *CMX973_VGA_Gains[] = {new Menu::menuValue<IF_GAIN>(IFGainNames[IF_GAIN_0], IF_GAIN_0, changeCMX973Gain, focusEvent),
                              new Menu::menuValue<IF_GAIN>(IFGainNames[IF_GAIN_MINUS6], IF_GAIN_MINUS6, changeCMX973Gain, focusEvent),
                              new Menu::menuValue<IF_GAIN>(IFGainNames[IF_GAIN_MINUS12], IF_GAIN_MINUS12, changeCMX973Gain, focusEvent),
                              new Menu::menuValue<IF_GAIN>(IFGainNames[IF_GAIN_MINUS18], IF_GAIN_MINUS18, changeCMX973Gain, focusEvent)};

Menu::select<IF_GAIN> &CMX973VGAGainMenu =
    *new Menu::select<IF_GAIN>("VGA Gain", config.hw.cmx973_vga, sizeof(CMX973_VGA_Gains) / sizeof(prompt *), CMX973_VGA_Gains, doNothing, updateEvent);

Menu::select<IF_GAIN> &CMX973VGBGainMenu =
    *new Menu::select<IF_GAIN>("VGB Gain", config.hw.cmx973_vgb, sizeof(CMX973_VGB_Gains) / sizeof(prompt *), CMX973_VGB_Gains, doNothing, updateEvent);

result changeCMX973Gain(eventMask e, navNode &nav, Menu::prompt &item) {
    IF_GAIN gain = ((Menu::menuValue<IF_GAIN> &)item).target();

    if ((Menu::select<IF_GAIN> *)nav.target == &CMX973VGAGainMenu) {
        config.hw.cmx973_vga = gain;
    } else {
        config.hw.cmx973_vgb = gain;
    }

    if (!dsp::dsp_config.agc_enabled) {
        if_gain(RF_DIRECTION_RX, config.hw.cmx973_vga,
                config.hw.cmx973_vgb); // Apply immediately if digital AGC is disabled. Otherwise it is controlled in agc.cpp
    }
    return proceed;
}

result resetPLL(eventMask e) {
    lo_setup();
    lo_freq(0, radio::mixers[0].getLo());
    lo_freq(1, radio::mixers[1].getLo());
    return proceed;
}

// TODO: Calculate SD_CARD_MAX_RATE in initialization procedure

MENU(boardMenu, "Hardware", doNothing, noEvent, noStyle, SUBMENU(CMX973VGAGainMenu), SUBMENU(CMX973VGBGainMenu),
     FIELD(adf4350Params.channel_spacing, "Channel spacing", "", 500, 100000, 500, 0, resetPLL, exitEvent, noStyle),
     FIELD(adf4350Params.charge_pump_current, "CP current", "", 0, 15, 1, 0, resetPLL, exitEvent, noStyle),
     FIELD(adf4350Params.cycle_slip_reduction_enable, "Slip reduction", "", 0, 1, 1, 0, resetPLL, exitEvent, noStyle),
     FIELD(adf4350Params.low_spur_mode_enable, "Low spur", "", 0, 1, 1, 0, resetPLL, exitEvent, noStyle),
     FIELD(adf4350Params.reference_div2_enable, "Ref div2", "", 0, 1, 1, 0, resetPLL, exitEvent, noStyle),
     FIELD(adf4350Params.reference_doubler_enable, "Ref x2", "", 0, 1, 1, 0, resetPLL, exitEvent, noStyle),
     FIELD(config.hw.dac_offset, "DAC offset", "", 0, 3000, 1, 100, doNothing, noEvent, noStyle),
     FIELD(config.hw.dac_off_balance, "DAC offset balance", "", -500, 500, 1, 10, doNothing, noEvent, noStyle),
     FIELD(config.hw.sd_write_max_kbps, "SD card max. write speed", "Kbps.", 500, SD_CARD_WRITE_MAX_KBPS, 25, 0, doNothing, noEvent, noStyle)

)
} // namespace boardUI
