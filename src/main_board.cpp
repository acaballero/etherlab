//
// Created by Angel Dust on 23/06/2021.
//

#include "main_board.h"
#include "Signal.h"
#include "dsp/dsp.h"
#include "dsp/dsp_common.h"
#include "hw/board/board_v2.h"
#include "hw/hw_config.h"
#include "os/task_manager.h"
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
#include "../lib/printf/printf.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_gpio.h"
#include "types.h"
#include "dsp/dsp_tasks.h"
#include <sys/_stdint.h>

namespace main_board {

bool set_mode(MODE mode);

GPIOPin powCtrlDataPin(POW_CTRL_DATA_PIN, POW_CTRL_DATA_PORT, GPIO_MODE_OUTPUT_PP);
GPIOPin powCtrlSetPin(POW_CTRL_SET_PIN, POW_CTRL_SET_PORT, GPIO_MODE_OUTPUT_PP);
GPIOPin powCtrlClkPin(POW_CTRL_CLK_PIN, POW_CTRL_CLK_PORT, GPIO_MODE_OUTPUT_PP);

MCP23017Pin mutePin(GPIOEXP_MUTE, MCP23017_PORTA, &hmcp02, GPIO_MODE_OUTPUT_PP);
ShiftReg PowControlShiftReg(&powCtrlDataPin, &powCtrlClkPin, &powCtrlSetPin);

GPIO_PinState mute = GPIO_PIN_RESET;
bool analog_mute_enabled = true;

MODE last_mode = MODE_NONE;

Signal mode_signal{"mode_signal"};
Signal if_filter_signal{"if_filter_signal"};

radio::FRONTEND_PATH frontend_path = radio::FRONTEND_PATH_LNA;

st_modulation_mode modes[] = {{CW, false}};

battery::BATTERY_STATUS battery_status = battery::BATTERY_STATUS_UNDEFINED;

void enable_analog_mute(bool b) {
    analog_mute_enabled = b;
    if (!b) {
        set_mute(GPIO_PIN_RESET);
    } else {

        // set_mute(info.in_squelch && info.level > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

void s_strength_callback(void *, const void *args) {

    if (analog_mute_enabled) {
        sstrength::st_sstrength_info info = *((sstrength::st_sstrength_info *)args);
        set_mute(info.in_squelch && info.level > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

void on_dsp_event(st_dsp_params *status) {
    switch (status->status) {

        case DSP_STATUS_RUNNING:
        case DSP_STATUS_PENDING:

            break;

        case DSP_STATUS_STOPPED:
        default:

            break;
    }
}

void check_status() {

    // If the battery voltage is below 6v means that we're on USB power so no bother with the temp or alerting

    if (battery_status != battery::battery_info.status) {
        // Shuts down power amp if battery is low
        if (battery::battery_info.voltage > 6 && battery::battery_info.status == battery::BATTERY_STATUS_LOW) {
            status::pop_alert(status::INFO, "Battery low");
            set_modulation_mode(config.modulation, true);
        }

        battery_status = battery::battery_info.status;
    }

    // Shuts down/turns on power amp bias as needed
    bool biased = ISTX && power_amp::status == power_amp::OK && rf_coupler::info.swr < rf_coupler::HIGH_SWR &&
                  rf_coupler::info.p_for_dbm < config.max_power_dbm && battery::battery_info.status != battery::BATTERY_STATUS_LOW;

    if (!biased && power_amp::status == power_amp::OK) {

        power_amp::shutdown();
        if (battery::battery_info.voltage > 6 && power_amp::status == power_amp::HIGH_TEMP) {
            status::pop_alert(status::ERROR, "Power amp high temperature");
        }

        if (rf_coupler::info.swr >= rf_coupler::HIGH_SWR) {
            status::pop_alert(status::ERROR, "High SWR");
        }

        if (rf_coupler::info.p_for_dbm >= config.max_power_dbm) {
            status::pop_alert(status::ERROR, "HPA max power exceeded");
        }
    }

    setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_POW_AMP_BIAS, biased, true);
}

void power_amp_status_callback(void *, const void *) {
    check_status();
}

void rf_coupler_info_callback(void *, const void *) {
    check_status();
}

void battery_callback(void *, const void *) {
    check_status();
}

void if_filter_signal_callback(void *, const void *) {
    if (config.mode == DIGITAL_RX) { // Restart receive task
        dsp_restart();
    }
}

void init() {

    setup_board_peripherals();

    power_amp::status_signal.add(NULL, power_amp_status_callback);
    rf_coupler::rf_coupler_signal.add(NULL, rf_coupler_info_callback);
    rf_coupler::set_offset(config.coupler_0db_mv);
    sstrength::squelch_signal.add(NULL, s_strength_callback);
    battery::battery_signal.add(NULL, battery_callback);
    main_board::if_filter_signal.add(nullptr, if_filter_signal_callback);
    set_modulation_mode(config.modulation, true);

    // Standby led
    setGPIOExpPin(&hmcp03, MCP23017_PORTB, GPIOEXP_FPANEL_STBY_LED, true, true);

    set_frontend_path(config.frontend_path);
}

void set_frontend_path(radio::FRONTEND_PATH path) {
    if (path != radio::FRONTEND_PATH_AUTO) {
        frontend_path = path;
    } else {
        frontend_path = radio::FRONTEND_PATH_LNA;
    }
    update();
}

radio::FRONTEND_PATH get_frontend_path() {
    return frontend_path;
}

int get_frontend_gain() {
    switch (main_board::get_frontend_path()) {
        case radio::FRONTEND_PATH_ATT:
            return -10;
        case radio::FRONTEND_PATH_THRU:
            return 0;
        case radio::FRONTEND_PATH_LNA:
            return 20;
        default:
            return -100;
    }
}

bool change_frontend_gain(int direction) {
    if (config.frontend_path == radio::FRONTEND_PATH_AUTO) {
        radio::FRONTEND_PATH current_path = get_frontend_path();
        if ((current_path > radio::FRONTEND_PATH_ATT && direction < 0) || (current_path < radio::FRONTEND_PATH_LNA && direction > 0)) {
            set_frontend_path((radio::FRONTEND_PATH)(current_path + direction));

            return true;
        }
    }
    return false;
}

bool alc_enabled() {
    return ISTX;
}

void setGPIO() {

    bool changed = false;

    // Set the +5v (20 ma.) RX/TX pin in the MCP32017
    changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_RX, !ISTX, false);
    changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_5VIF_RX, ISTX, false); // Inverted logic
    changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_5VIF_TX, ISTX, false);

    // Set the +5v (60 ma.) for the LNA in RX (inverted logic)
    changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_LNA, !(!ISTX && get_frontend_path() == radio::FRONTEND_PATH_LNA), false);
    // Frontend pass-thru path
    changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FRONT_THRU, !ISTX && get_frontend_path() == radio::FRONTEND_PATH_THRU, false);
    // Frontend attenuator path
    changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FRONT_ATTENUATOR, !ISTX && get_frontend_path() == radio::FRONTEND_PATH_ATT, false);

    // Set digital TX or analog RX&TX signal between the 1st and 2nd mixers
    changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTA, GPIOEXP_ANALOG_RXTX_DIGITAL_TX_SWITCH, config.mode != DIGITAL_TX, false);

    // ALC (Automatic level control) is Off during RX
    changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_ALC, alc_enabled(), false);

    // AGC (Automatic gain control) is Off in TX
    // In FM and AM the RSSI signal from the log amplifier (of the particular demodulator board) is fed to the RSSI level adapter and then to the AGC board just
    // before the level detector mosfet
    changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_AGC, !ISTX && config.agc_enabled, false);

    // Experimental: In DIGITAL modes, the RSSI is the output from the logamp that's fed with the 1st IF.
    changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_IF_RSSI_5V, !ISANALOG && !ISTX, false);

    changed = changed | setGPIOExpPin(&hmcp03, MCP23017_PORTA, GPIOEXP_FPANEL_TX_LED, ISTX, false);

    if (changed) {
        commitGPIOExpPort(&hmcp01, MCP23017_PORTA);
        commitGPIOExpPort(&hmcp01, MCP23017_PORTB);
        commitGPIOExpPort(&hmcp02, MCP23017_PORTA);
        commitGPIOExpPort(&hmcp02, MCP23017_PORTB);
        commitGPIOExpPort(&hmcp03, MCP23017_PORTA);
    }
}

st_modulation_mode *find_modulation_info(MODULATION_MODE modulation) {
    st_modulation_mode *mode = nullptr;

    for (auto m : modes) {
        if (m.modulation == modulation) {
            mode = &m;
            break;
        }
    }

    return mode;
}

bool allow_modulation_in_mode(MODE mode, MODULATION_MODE modulation) {
    st_modulation_mode *mode_info = find_modulation_info(modulation);
    if (!mode_info || mode_info->analog_allowed ||
        !ANALOGMODE(mode)) { // Not all modes are worth storing in the extended info struct. Just being lazy as hell, right?
        return true;
    }
    return false;
}

void toggle_dsp() {
    if (!ISTX) {
        if (config.mode != DIGITAL_RX) {
            set_mode(DIGITAL_RX);
        } else {
            if (allow_modulation_in_mode(config.mode, config.modulation)) {
                set_mode(ANALOG_RX);
            } else {
                status::pop_alert(status::WARN, "Modulation disabled in analog");
            }
        }
    }
}

bool _set_mode(MODE mode, bool force) {

    volatile static bool setting_mode;

    if (setting_mode) {
        return false;
    }

    setting_mode = true;

    bool changed = false;

    MODE current_mode = config.mode; // Remember last mode for toggling back

    if (force || mode != current_mode || last_mode == MODE_NONE) {

        LOG("_setMode: mode: %s, current: %s, forced: %b\n", radio::modeNames[mode], radio::modeNames[current_mode], force);

        if (TXMODE(mode)) {
            if (!radio::tx_enabled()) {
                status::pop_alert(status::WARN, "TX disabled for current band");
                return false;
            }

            if (!config.hpa_enabled) {
                status::pop_alert(status::WARN, "Power amplifier disabled");
            }
        }

        changed = last_mode != mode;
        config.mode = mode;

        GPIO_PinState muteState = get_mute();

        set_mute(GPIO_PIN_SET);

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

            // Leave +12v as it was, in case this is executed twice to prevent it from enable/disable/enable
            if (config.hpa_enabled) {
                power_ctrl = (config.power_ctrl & POWCRL_P12) | power_ctrl;
            }

            setGPIO();
            set_power_ctrl(power_ctrl, false);

            if (config.hpa_enabled) {
                power_ctrl = config.power_ctrl | POWCRL_P12;
                set_power_ctrl(power_ctrl, force);
            }

            HAL_Delay(10);

            if (ISANALOG) {

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

                if_gain(RF_DIRECTION_RX, IF_GAIN_MINUS18, (config.modulation == FM || config.modulation == WFM) ? IF_GAIN_MINUS6 : IF_GAIN_MINUS30);
            }

            if (config.hpa_enabled) {
                power_amp::enable();
            }

            rf_coupler::enable();

        } else { // RX

            rf_coupler::disable();

            if (config.hpa_enabled) {
                power_amp::disable();
            }

            HAL_Delay(10);

            power_ctrl = config.power_ctrl & ~POWCRL_P12;
            set_power_ctrl(power_ctrl, false);

            if (battery::battery_info.status == battery::BATTERY_STATUS_VERY_LOW) {
                // In low battery mode. Disable all power rails
                power_ctrl = POWCRL_P5;
            } else {

                power_ctrl = POWCRL_PB1 | POWCRL_P5 | POWCRL_PA2 | (config.modulation == SSB_LSB || config.modulation == SSB_USB ? POWCRL_PC2 : 0);
            }

            set_power_ctrl(power_ctrl, force);

            HAL_Delay(10);

            setGPIO();

            fft_config(config.fft.span);
        }

        set_filter();

        if (config.mode == DIGITAL_TX) {

            // Turn off 2nd and 3rd mixers LOs
            lo_enable(1, 0);
            lo_enable(2, 0);

        } else if (config.mode == DIGITAL_RX) {

            // Turn off 3rd mixer LO
            lo_enable(2, 0);
            set_if_filter(config.if_filter);

        } else {

            lo_enable(1, 1);
            if ((config.modulation == SSB_LSB || config.modulation == SSB_USB)) {
                lo_enable(2, 1);
            }
            set_if_filter(config.if_filter);
        }

        if (ISTX) {
            // We just want to see the signal being sent
            fft_config(max2(radio::get_bandwidth_hz() * 4, 40000));
        }

        if (changed) {
            LOG("Changed: Last mode %s | current %s\n", radio::modeNames[last_mode], radio::modeNames[current_mode]);
            last_mode = current_mode;
            mode_signal.emit(nullptr);
        }

        set_mute(muteState);
    }

    setting_mode = false;
    return true;
}

bool set_mode(MODE mode) {
    // LOG("------ [BEGIN] setMode %s ------\n", radio::modeNames[mode]);
    bool b = false;

    if (_set_mode(mode, false)) {
        set_modulation_mode(config.modulation, true);
        b = true;
    }

    // LOG("------ [END] setMode %s: %d ------\n", radio::modeNames[mode], b);
    return b;
}

void sleep() {
    if (!ISTX) {
        set_power_ctrl(0, true);
        setGPIOExpPort(&hmcp01, MCP23017_PORTA, 0);
        setGPIOExpPort(&hmcp01, MCP23017_PORTB, 0xF8); // inverted logic in lines 3 - 7
        setGPIOExpPort(&hmcp02, MCP23017_PORTA, 0);
        setGPIOExpPort(&hmcp02, MCP23017_PORTB, 0);
        radio::if_filter = radio::IF_FILTER_NONE; // Forces re-setting the 2nd LO clock on wakeup
    }
}

void wakeup() {
    set_modulation_mode(config.modulation, true);
}

void update() {
    _set_mode(config.mode, true);
}

bool toggle_mode() {
    MODE mode;
    if (ISTX) {
        mode = ANALOGMODE(last_mode) ? ANALOG_RX : DIGITAL_RX;
    } else {
        // TODO: Still only analog modulation for TX
        mode = ANALOG_TX;
    }

    return main_board::set_mode(mode);
}

void set_mute(GPIO_PinState muteState) {
    if (mute != muteState) {
        //  LOG("setMute: %d\n", static_cast<int>(muteState));
        mute = muteState;
        if (mutePin.set(muteState) != HAL_OK) {
            status::pop_alert(status::ERROR, "Error setting mute");
        }
    }
}

GPIO_PinState get_mute() {
    return mute;
}

MODULATION_MODE get_modulation_mode() {
    return config.modulation;
}

void set_modulation_mode(MODULATION_MODE mod_val, bool force) {

    volatile static bool setting_modulation;

    if (setting_modulation) {
        return;
    }

    if (mod_val >= MODULATION_MODE_ALL) {
        return;
    }

    setting_modulation = true;

    if (battery::battery_info.status == battery::BATTERY_STATUS_VERY_LOW) { // Disble all if low power
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

        set_mute(GPIO_PIN_SET);

        if (!allow_modulation_in_mode(config.mode, config.modulation)) {
            // Only-digital modes allowed for some modulations

            config.mode = ISTX ? DIGITAL_TX : DIGITAL_RX;
        }

        // Update mode if modulation changes so power lines are set according to the new modulation
        _set_mode(config.mode, changed);

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
                //  changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_2ND_15KHZ_FILTER, ISANALOG, false);

                changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FM_DETECTOR, ISTX || !ISANALOG,
                                                  false); // When low, it powers up the +5v rail that goes into the FM detector board
                changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FM_MODULATOR, !(config.mode == ANALOG_TX), false);
                changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_AM_DETECTOR, true,
                                                  false); // When low, it powers up the +5v rail that goes into the AM detector board

                break;
            case AM:
                changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_RSSI_LEVEL_ADAPTER, !ISTX, false);
                changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_10MHHZ_MIXER, false, false);
                // changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_2ND_15KHZ_FILTER, ISANALOG, false);

                changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_AM_DETECTOR, ISTX || !ISANALOG, false);
                changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FM_DETECTOR, true, false);
                changed = changed | setGPIOExpPin(&hmcp01, MCP23017_PORTB, GPIOEXP_FM_MODULATOR, true, false);

                break;

            default:
                changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTA, GPIOEXP_RSSI_LEVEL_ADAPTER, !ISTX && !ISANALOG, false);

                // The 10Mhz mixer can be fed either by its 10Mhz TCXO or a synthesized Si5351 output. Currently, we use
                // the Si5351 only when the selected IF filter is not at 10Mhz, so we're setting this GPIO pin in the filter
                // selection function.
                // TODO: If the Si5351 output is good enough for this (my concern is it may radiate and interfere since the IF mixer
                // is far away and there's a long run of micro coax), the 10Mhz TCXO should remain unused even for the 10Mhz IF filter
                // changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_10MHHZ_MIXER, true, false);

                // Close the 2nd 15Khz IF filter which is only used for FM/AM analog detectors
                // changed = changed | setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_2ND_15KHZ_FILTER, false, false);

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

            radio::update_freq(); // So intermediate frequencies are recalculated

            mode_signal.emit(nullptr);
        }

        set_mute(muteState);
    }

    setting_modulation = false;
}

void set_power_rails(uint8_t value, bool force, bool oneByOne) {

    if (force || value != config.power_ctrl) {

        config.power_ctrl = value;

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

                    if (set) {
                        HAL_Delay(1); // if has changed from 0 to 1
                    }
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

void set_power_ctrl(uint8_t value, bool force) {

    uint8_t curr_ctrl_bits = PowControlShiftReg.getValue();

    // 1: Turn off the lines that will be off
    set_power_rails(curr_ctrl_bits & value, true, false);

    // 1: Turn on remaining lines one by one
    set_power_rails(value, force, true);
}

bool setGPIOExpPin(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort, uint8_t pin, bool set) {
    return setGPIOExpPin(hmcp, mcpPort, pin, set, true);
}

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
        status::pop_alert(status::ERROR, "GPIO expander port error");
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
        LOG("Setting filter GPIO for band '%s'\n", radio::bandNames[new_filter]);
        hmcp01.gpio[MCP23017_PORTA] = new_bits;
        mcp23017_write_gpio(&hmcp01, MCP23017_PORTA);
    }
}

void set_if_filter(radio::IF_FILTER fil) {

    // TODO: Save current filter band and change only if it's changed
    // TODO: switch off entire analog IF section in digital TX mode

    radio::IF_FILTER new_filter;

    if (fil == radio::IF_FILTER_AUTO || !radio::is_filter_allowed(fil)) {
        new_filter = radio::band_if_filter();
    } else {
        new_filter = fil;
    }

    if (new_filter != radio::if_filter) {

        LOG("Setting IF filter %s\n", radio::IFFilterNames[new_filter]);

        radio::if_filter = new_filter;

        // Switch off all (analog) filters
        uint8_t pin;
        int n_filters = sizeof(radio::if_filters) / sizeof(radio::if_filters[0]);
        for (int i = 0; i < n_filters; i++) {
            pin = radio::if_filters[i].pin;

            if (radio::if_filters[i].analog_available) {
                setGPIOExpPin(&hmcp01, MCP23017_PORTA, pin, false, false);
            }
        }

        pin = radio::if_filters[radio::if_filter].pin;

        if (radio::if_filters[radio::if_filter].analog_available) { // Digital filters don't have a GPIO pin
            setGPIOExpPin(&hmcp01, MCP23017_PORTA, pin, true, false);
        }
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
            int offset = (int)(radio::if_filters[radio::if_filter].bandwidth / 2) + 500; // +500 to account for the skirt

            lo_enable(2, 1);
            lo_freq(2, radio::if_filters[radio::if_filter].freq + offset);
            //}

            // commitGPIPExpPin(&hmcp02, MCP23017_PORTB);
        } else {
            lo_enable(2, 0);
            // setGPIOExpPin(&hmcp02, MCP23017_PORTB, GPIOEXP_10MHHZ_MIXER, true, true);
        }

        if_filter_signal.emit(nullptr);
    }
}
} // namespace main_board
