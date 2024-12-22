//
// Created by Angel Dust on 23/06/2021.
//

#include "main_board.h"
#include "s_strength.h"
#include "rf_coupler.h"
#include "config.h"
#include "ShiftReg.h"
#include "battery.h"
#include "power_amp.h"
#include "status.h"
#include "../lib/IOPin/GPIOPin.h"
#include "../lib/IOPin/MCP23017Pin.h"
#include "radio.h"

namespace main_board {

bool setMode(MODE mode);

GPIOPin powCtrlDataPin(POW_CTRL_DATA_PIN, POW_CTRL_DATA_PORT, GPIO_MODE_OUTPUT_PP);
GPIOPin powCtrlSetPin(POW_CTRL_SET_PIN, POW_CTRL_SET_PORT, GPIO_MODE_OUTPUT_PP);
GPIOPin powCtrlClkPin(POW_CTRL_CLK_PIN, POW_CTRL_CLK_PORT, GPIO_MODE_OUTPUT_PP);

MCP23017Pin mutePin(GPIOEXP_MUTE, MCP23017_PORTA, &hmcp02, GPIO_MODE_OUTPUT_PP);
ShiftReg PowControlShiftReg(&powCtrlDataPin, &powCtrlClkPin, &powCtrlSetPin);

GPIO_PinState mute = GPIO_PIN_RESET;

battery::BATTERY_STATUS battery_status = battery::BATTERY_STATUS_UNDEFINED;

void s_strength_callback(void *thisptr, void *args) {
        sstrength::st_sstrength_info info = *((sstrength::st_sstrength_info *)args);
        setMute(info.in_squelch && info.level > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void s_level_callback(void *thisptr, void *args) {
        if (false) { // TODO: If frontend amplification is set to AUTO
                float s_level = *((float *)args);
                if (s_level > 11 && config.frontend_path != radio::FRONTEND_PATH_ATT) {
                        config.frontend_path = (radio::FRONTEND_PATH)(config.frontend_path - 1);
                }
        }
}

void check_status() {

        // If the battery voltage is below 6v means that we're on USB power so no bother with the temp or alerting

        if (battery_status != battery::battery_info.status) {
                // Shuts down power amp if battery is low
                if (battery::battery_info.voltage > 6 && battery::battery_info.status == battery::BATTERY_STATUS_LOW) {
                        status::handleError(status::ST_INFO, "Battery low");
                        setModulationMode(config.modulation, true);
                }

                battery_status = battery::battery_info.status;
        }

        if (battery::battery_info.voltage > 6 && power_amp::status == power_amp::HIGH_TEMP) {
                status::handleError(status::ST_ERROR, "Power amp high temperature");
        }

        if (rf_coupler::info.swr >= rf_coupler::HIGH_SWR) {
                status::handleError(status::ST_ERROR, "High SWR");
        }

        if (rf_coupler::info.p_for_dbm >= config.max_power_dbm) {
                status::handleError(status::ST_ERROR, "HPA max power exceeded");
        }

        // Shuts down/turns on power amp bias as needed
        bool biased = ISTX && power_amp::status == power_amp::OK && rf_coupler::info.swr < rf_coupler::HIGH_SWR &&
                      rf_coupler::info.p_for_dbm < config.max_power_dbm && battery::battery_info.status != battery::BATTERY_STATUS_LOW;

        setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_POW_AMP_BIAS, biased, true);
}

void power_amp_status_callback(void *thisptr, void *args) { check_status(); }

void rf_coupler_info_callback(void *thisptr, void *args) { check_status(); }

void battery_callback(void *thisptr, void *args) { check_status(); }

void init() {
        power_amp::status_signal.add(NULL, power_amp_status_callback);
        rf_coupler::rf_coupler_signal.add(NULL, rf_coupler_info_callback);
        rf_coupler::set_offset(config.coupler_0db_mv);
        sstrength::squelch_signal.add(NULL, s_strength_callback);
        sstrength::s_strength_signal.add(NULL, s_level_callback);
        battery::battery_signal.add(NULL, battery_callback);
        setModulationMode(config.modulation, true);

        // Standby led
        setGPIOExpPin(&hmcp03, MCP23017_PORTB, GPIOEXP_FPANEL_STBY_LED, true, true);
}

void setGPIO() {

        bool changed = false;

        // Set the +5v (20 ma.) RX/TX pin in the MCP32017
        changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_RX, !ISTX, false);
        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_5VIF_RX, ISTX, false); // Inverted logic
        changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_5VIF_TX, ISTX, false);

        // Set the +5v (60 ma.) for the LNA in RX (inverted logic)
        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_LNA, !(!ISTX && config.frontend_path == radio::FRONTEND_PATH_LNA), false);
        // Frontend pass-thru path
        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FRONT_THRU, !ISTX && config.frontend_path == radio::FRONTEND_PATH_THRU, false);
        // Frontend attenuator path
        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FRONT_ATTENUATOR, !ISTX && config.frontend_path == radio::FRONTEND_PATH_ATT, false);

        // Set digital TX or analog RX&TX signal between the 1st and 2nd mixers
        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTA, GPIOEXP_ANALOG_RXTX_DIGITAL_TX_SWITCH, config.mode != DIGITAL_TX, false);

        // ALC (Automatic level control) is Off in RX
        changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_ALC, ISTX, false);

        // AGC (Automatic gain control) is Off in TX
        // In FM and AM the RSSI signal from the log amplifier is fed to the RSSI level adapter and then to the AGC board just before the
        // level detector mosfet
        changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_AGC, !ISTX && config.agc_enabled, false);

        changed = changed | setGPIOExpPin(&hmcp03, MCP23017_PORTA, GPIOEXP_FPANEL_TX_LED, ISTX, false);

        if (changed) {
                commitGPIOExpPort(&hmcp01, MCP23017_PORTA);
                commitGPIOExpPort(&hmcp01, MCP23017_PORTB);
                commitGPIOExpPort(&hmcp02, MCP23017_PORTA);
                commitGPIOExpPort(&hmcp02, MCP23017_PORTB);
                commitGPIOExpPort(&hmcp03, MCP23017_PORTA);
        }
}

bool _setMode(MODE mode, bool force) {

        if (force || mode != config.mode) {

                if (TXMODE(mode) && !radio::tx_enabled()) {
                        status::handleError(status::ST_WARN, "TX disabled for current band");
                        return false;
                }

                config.mode = mode;

                GPIO_PinState muteState = mute;

                setMute(GPIO_PIN_SET);

                /**
                 *  Change power lines according to RX/TX modes.
                 *  Sequencing:
                 *      - TX->RX:
                 *        Power amp bias (off)
                 *        Power amp +12v (off)
                 *        TX power rail (off)
                 *        RX power rail (on)
                 *      - RX->TX:
                 *        RX power rail (off)
                 *        TX power rail (on)
                 *        Power amp +12v (on)
                 *        Power amp bias (on)
                 *
                 */

                uint8_t power_ctrl;

                if (ISTX) {

                        power_ctrl = POWCRL_PB1 | POWCRL_P5 | POWCRL_PA1 | (config.modulation == SSB_LSB || config.modulation == SSB_USB ? POWCRL_PC1 : 0);
                        power_ctrl = (config.power_ctrl & POWCRL_P12) |
                                     power_ctrl; // Leave +12v as it was, in case this is executed twice to prevent it from enable/disable/enable

                        setPowerCtrl(power_ctrl, false);

                        setGPIO();

                        power_ctrl = config.power_ctrl | POWCRL_P12;
                        setPowerCtrl(power_ctrl, force);

                        HAL_Delay(10);

                        if (ISANALOG) {
                                // We just want to see the signal being sent
                                fft_config(FFT_MIN_SPAN);

                                /*

                                   In TX we reduce the quadrature demodulator gain to prevent saturation, but
                                   the gain must be much lower in modes other than FM. This is why:

                                   The FM signal is injected after the 2nd mixer and the signal gets into
                                   the DSP very attenuated since it just couples by the capacitance in AMP2,
                                   which is switched off in this mode.
                                   On the other hand, the signal from the analog SSB modulator gets to the DSP
                                   after being amplified in AMP3.


                                                                                    FM modulator
                                                                                          |
                                                                                          v
                                   SSB modulator -> FLT -> AMP3 -> AMP2 -> 2nd mixer -> FLT -> AMP1 -> 1st mixer -> PA
                                                                 |
                                                                 v
                                                                DSP


                                */

                                if_gain(RF_DIRECTION_RX, IF_GAIN_MINUS18,
                                        (config.modulation == FM || config.modulation == WFM) ? IF_GAIN_MINUS6 : IF_GAIN_MINUS30);
                        }

                        if (config.hpa_enabled)
                                power_amp::enable();
                        rf_coupler::enable();

                } else {

                        rf_coupler::disable();
                        if (config.hpa_enabled)
                                power_amp::disable();

                        HAL_Delay(10);

                        power_ctrl = config.power_ctrl & ~POWCRL_P12;
                        setPowerCtrl(power_ctrl, false);

                        if (battery::battery_info.status == battery::BATTERY_STATUS_LOW) {
                                // In low battery mode. Disable all power rails
                                power_ctrl = POWCRL_P5;
                        } else {

                                power_ctrl =
                                    POWCRL_PB1 | POWCRL_P5 | POWCRL_PA2 | (config.modulation == SSB_LSB || config.modulation == SSB_USB ? POWCRL_PC2 : 0);
                        }
                        setPowerCtrl(power_ctrl, force);

                        HAL_Delay(10);

                        setGPIO();

                        // Restore configured gain of the quadrature demodulator
                        if_gain(RF_DIRECTION_RX, config.hw.cmx973_vga, config.hw.cmx973_vgb);

                        fft_config(config.fft.span);
                }

                set_filter();

                if (config.mode == DIGITAL_TX) {
                        // Turn off 2nd and 3rd mixers LOs
                        lo_enable(1, 0);
                        lo_enable(2, 0);
                } else {
                        lo_enable(1, 1);
                        // lo_enable(2, 1);
                        set_if_filter(config.if_filter);
                }

                setMute(muteState);
        }

        return true;
}

void sleep() {
        if (!ISTX) {
                setPowerCtrl(0, true);
                setGPIOExpPort(&hmcp01, MCP23017_PORTA, 0);
                setGPIOExpPort(&hmcp01, MCP23017_PORTB, 0xF8); // inverted logic in lines 3 - 7
                setGPIOExpPort(&hmcp02, MCP23017_PORTA, 0);
                setGPIOExpPort(&hmcp02, MCP23017_PORTB, 0);
                radio::if_filter = radio::IF_FILTER_NONE; // Forces re-setting the 2nd LO clock on wakeup
        }
}

void wakeup() { setModulationMode(config.modulation, true); }

void update() { _setMode(config.mode, true); }

bool setMode(MODE mode) {
        if (_setMode(mode, false)) {
                setModulationMode(config.modulation, true);
                return true;
        }

        return false;
}

void setMute(GPIO_PinState muteState) {
        if (mute != muteState) {
                mute = muteState;
                mutePin.set(muteState);
        }
}

GPIO_PinState getMute() { return mute; }

MODULATION_MODE getModulationMode() { return config.modulation; }

void setModulationMode(int mod_val, bool force) {

        if (battery::battery_info.status == battery::BATTERY_STATUS_LOW) {
                setGPIOExpPort(&hmcp01, MCP23017_PORTA, 0x00);
                setGPIOExpPort(&hmcp01, MCP23017_PORTB, 0xFF);
                setGPIOExpPort(&hmcp02, MCP23017_PORTA, 0x00);
                setGPIOExpPort(&hmcp02, MCP23017_PORTB, 0xFF);
        } else if (force || mod_val != (int)config.modulation) {

                bool changed = mod_val != (int)config.modulation;

                config.modulation = (MODULATION_MODE)mod_val;

                // In RX, set the mute to avoid the "thump" in the speakers if
                // a transient in the audio signal occurs due to a change in power lines.
                // For example, when going from AM/FM to SSB, the bias change in the
                // switches in the signal path is causing one of such transients

                GPIO_PinState muteState = mute;

                setMute(GPIO_PIN_SET);

                // Set RX/TX mode to set the power lines according to the new modulation
                _setMode(config.mode, true);

                // Set some GPIO pins according to the new modulation
                // NOTE: hmcp01 has negative logic

                /**
                 * In digital mode, we still want to enable the analog detectors since there's still no DSP signal strength detector.
                 */

                switch (config.modulation) {
                case FM:
                case WFM:
                        changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_RSSI_LEVEL_ADAPTER, !ISTX, false);
                        changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_10MHHZ_MIXER, false, false);
                        changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_2ND_15KHZ_FILTER, true, false);
                        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FM_DETECTOR, ISTX,
                                                          false); // When low, it powers up the +5v rail that goes into the FM detector board
                        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FM_MODULATOR, !(config.mode == ANALOG_TX), false);
                        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_AM_DETECTOR, true,
                                                          false); // When low, it powers up the +5v rail that goes into the AM detector board

                        break;
                case AM:
                        changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_RSSI_LEVEL_ADAPTER, !ISTX, false);
                        changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_10MHHZ_MIXER, false, false);
                        changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_2ND_15KHZ_FILTER, true, false);
                        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_AM_DETECTOR, ISTX, false);
                        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FM_DETECTOR, true, false);
                        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FM_MODULATOR, true, false);

                        break;

                default:
                        changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_RSSI_LEVEL_ADAPTER, false, false);

                        // The 10Mhz mixer can be fed either by its 10Mhz TCXO or a synthesized Si5351 output. Currently, we use
                        // the Si5351 only when the selected IF filter is not at 10Mhz, so we're setting this GPIO pin in the filter
                        // selection function.
                        // TODO: If the Si5351 output is good enough for this (my concern is it may radiate and interfere since the IF mixer
                        // is far away and there's a long run of micro coax), the 10Mhz TCXO should remain unused even for the 10Mhz IF filter
                        // changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_10MHHZ_MIXER, true, false);

                        // Close the 2nd 15Khz IF filter which is only used for FM/AM analog detectors
                        changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_2ND_15KHZ_FILTER, false, false);

                        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_AM_DETECTOR, true, false);
                        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FM_DETECTOR, true, false);
                        changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FM_MODULATOR, true, false);

                        break;
                }

                if (changed) {
                        commitGPIOExpPort(&hmcp01, MCP23017_PORTA);
                        commitGPIOExpPort(&hmcp01, MCP23017_PORTB);
                        commitGPIOExpPort(&hmcp02, MCP23017_PORTA);
                        commitGPIOExpPort(&hmcp02, MCP23017_PORTB);
                        radio::update_freq();
                        // Set the mute in its original state
                        HAL_Delay(100); // skip the audio transient if any
                }

                setMute(muteState);
        }
}

void setPowerCtrl(uint8_t value, bool force, bool oneByOne) {

        if (force || value != config.power_ctrl) {

                config.power_ctrl = value;

                HAL_Delay(10);

                if (oneByOne) {

                        // Start power lines one by one to limit inrush current (which can trip the regulator's overcurrent protection)
                        uint8_t ctrl = PowControlShiftReg.getValue(), new_ctrl = config.power_ctrl, mask = 1;
                        uint8_t current_ctrl = ctrl;
                        bool set;

                        for (uint8_t i = 0; i < 8; i++) {

                                set = new_ctrl & mask;
                                ctrl = set ? ctrl | mask : ctrl & ~mask;

                                // Change the register when there's a change, and only if it is from 0 to 1 (no need to go one by one when powering off) or it's
                                // the last bit
                                if (current_ctrl != ctrl && (set || i == 7)) {

                                        if (set)
                                                HAL_Delay(1); // if has changed from 0 to 1
                                        current_ctrl = ctrl;
                                        PowControlShiftReg.write(ctrl);
                                }

                                mask <<= 1;
                        }
                } else {
                        PowControlShiftReg.write(config.power_ctrl);
                }
        }
}

void setPowerCtrl(uint8_t value, bool force) {

        uint8_t curr_ctrl_bits = PowControlShiftReg.getValue();

        // 1: Turn off required lines
        setPowerCtrl(curr_ctrl_bits & value, true, false);

        // 1: Turn on remaining lines one by one
        setPowerCtrl(value, force, true);
}

bool setGPIOExpPin(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort, uint8_t pin, bool set) { return setGPIOExpPin(hmcp, mcpPort, pin, set, true); }

bool setGPIOExpPin(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort, uint8_t pin, bool set, bool commit) {

        uint8_t actual = hmcp->gpio[mcpPort];
        bool changed;

        if (set) {
                hmcp->gpio[mcpPort] |= (1 << pin);
        } else {
                hmcp->gpio[mcpPort] &= ~(1 << pin);
        }

        changed = actual != hmcp->gpio[mcpPort];

        commit = commit && (changed || !hmcp->initialized[mcpPort]);

        if (commit) {
                commitGPIOExpPort(hmcp, mcpPort);
        }

        return changed;
}

uint8_t getGPIOExpPort(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort) {

        mcp23017_read_gpio(hmcp, mcpPort);
        return hmcp->gpio[mcpPort];
}

void setGPIOExpPort(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort, uint8_t val) {

        hmcp->gpio[mcpPort] = val;
        commitGPIOExpPort(hmcp, mcpPort);
}

bool getGPIOExpPin(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort, uint8_t pin, bool set) {

        uint8_t data = getGPIOExpPort(hmcp, mcpPort);
        return data & (1 << pin);
}

void commitGPIOExpPort(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort) {
        uint32_t error = mcp23017_write_gpio(hmcp, mcpPort);
        if (error != I2CBB_ERROR_NONE) {
                status::handleError(status::ST_ERROR, "GPIO expander port error");
        }
}

void set_filter() {

        radio::BAND new_filter;

        // While transmitting, the filter is automatically set to the proper frequency band but, in RX mode,
        // we set the user selected filter

        if (config.filter == radio::BAND_AUTO || ISTX) {
                new_filter = radio::find_band(radio::get_frequency());
        } else {
                new_filter = (radio::BAND)config.filter;
        }

        radio::filter = new_filter;

        // Set the pins 0-3 of the PORT_A of the MCP23017
        uint8_t new_bits = (hmcp01.gpio[MCP23017_PORTA] & 0xFF0F) + radio::bands[radio::filter].filter_bank_code;

        if (hmcp01.gpio[MCP23017_PORTA] != new_bits) {
                hmcp01.gpio[MCP23017_PORTA] = new_bits;
                mcp23017_write_gpio(&hmcp01, MCP23017_PORTA);
        }
}

void set_if_filter(radio::IF_FILTER fil) {

        // TODO: Save current filter band and change only if it's changed
        // TODO: switch off entire analog IF section in digital TX mode

        radio::IF_FILTER new_filter;

        if (fil == radio::IF_FILTER_AUTO) {
                switch (config.modulation) {
                case SSB_USB:
                case SSB_LSB:
                case CW:
                        new_filter = radio::IF_FILTER_3KHZ;
                        break;
                case FM:
                case WFM:
                        if (radio::get_band() == radio::BAND_FM) {
                                new_filter = radio::IF_FILTER_150KHZ;
                        } else {
                                new_filter = radio::IF_FILTER_15KHZ;
                        }
                        break;
                case AM:
                        new_filter = radio::IF_FILTER_15KHZ;
                        break;
                default:
                        new_filter = radio::IF_FILTER_15KHZ;
                }
        } else {
                new_filter = fil;
        }

        if (new_filter != radio::if_filter) {

                radio::if_filter = new_filter;

                // Switch off all filters
                for (int i = 0; i < 3; i++) {
                        setGPIOExpPin(&hmcp01, MCP23017_PORTA, radio::if_filters[i].pin, false, false);
                }

                setGPIOExpPin(&hmcp01, MCP23017_PORTA, radio::if_filters[radio::if_filter].pin, true, false);
                commitGPIOExpPort(&hmcp01, MCP23017_PORTA);

                // The 10Mhz mixer can be fed either by its 10Mhz TCXO or a synthesized Si5351 output. Currently, we use
                // the Si5351 only when the selected IF filter is not at 10Mhz, so we're setting this GPIO pin here.
                // TODO: If the Si5351 output is good enough for this (my concern is it may radiate and interfere since the IF mixer
                // is far away and there's a long run of micro coax), the 10Mhz TCXO should remain unused even for the 10Mhz IF filter
                if ((config.modulation == SSB_LSB || config.modulation == SSB_USB) && config.mode != DIGITAL_TX) {

                        // if (radio::if_filter == radio::IF_FILTER_3KHZ) {
                        //     setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_10MHHZ_MIXER, true, false);
                        //     analog_if_freq(0);
                        // } else {
                        // setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_10MHHZ_MIXER, false, false);

                        // Apply an offset to put the left sideband onto the filter passband
                        int offset = (int)(radio::if_filters[radio::if_filter].bandwidth_khz * 1000 / 2) + 500; // +500 to account for the skirt

                        lo_enable(2, true);
                        lo_freq(2, radio::if_filters[radio::if_filter].freq + offset);
                        //}

                        // commitGPIPExpPin(&hmcp02, MCP23017_PORTB);
                } else {
                        lo_enable(2, false);
                        // setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_10MHHZ_MIXER, true, true);
                }
        }
}
} // namespace main_board
