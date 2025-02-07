//
// Created by Angel Dust on 15/07/2022.
//

#include <math.h>
#include "s_strength.h"
#include "periodic_task.h"
#include "config.h"
#include "signal.h"
#include <hw/stm32.h>
#include <stdio.h>

/* Definitions for the audio frequency signal strength meter */

#define S_STRENGTH_S9 2.89 // Voltage of S_STRENGTH_PIN at S9 level

#define S_STRENGTH_V_DB 0.013
#define S_STRENGTH_S1 (S_STRENGTH_S9 - (8 * 6 * S_STRENGTH_V_DB))

/*
 * Definitions for the RSSI level of the logarithmic amplifier
 *
 * The voltage increases by 2mv/dB
 *
 */

#define S_STRENGTH_S9_LOGAMP 2.89 /* S9 is -93 dBM */
#define S_STRENGTH_V_DB_LOGAMP 0.02
#define S_STRENGTH_S1_LOGAMP (S_STRENGTH_S9_LOGAMP - (8 * 6 * S_STRENGTH_V_DB_LOGAMP))

// Minimum time to turn on squelch after the signal goes under the threshold
#define SQUELCH_TIMEOUT_MS 500

namespace sstrength {

void check_signal_strength();

Signal s_strength_signal, squelch_signal;
st_sstrength_info info{.in_squelch = false, .level = config.squelch_level};
bool last_squelch_test;
float s_strength;
float s_level;
uint64_t last_activation_trigger_ms = 0;
periodic_task task(50, check_signal_strength);

// Converts dBs to S-units
float db_to_s_strength(float db) {

    // Each S-unit represents 6dB change in signal strength
    // S1=-141 dBm
    // From S9 onwards, each unit represents +10dB
    // So S9=-141dBm+6*8=-93dBm and S10=-83dBm

    float s;

    if (db < -141) {
        s = 1;
    } else if (db <= -93) {
        s = (((db + 141) / (141 - 93)) * 8) + 1;
    } else {
        s = 9 + ((db + 93) / 10);
    }

    // TODO: Currently the dB units used in the FFT are not calibrated and doesn't match the analog signal strength level
    // readings, so I subtract some constant to visually match it
    return max2(s - 6, 0);
}

float get_s_strength(bool filter, uint8_t channel) {

    uint16_t adcv = GetADCValue(&hadc3, channel, 3);

    float v = ((float)adcv / (float)MAX_ADC_VALUE) * (float)V_REF;

    // exponential filter
    float filter_factor = filter ? 0.3 : 0.95;

    s_strength = (s_strength - (filter_factor * (s_strength - v)));

    float vS1, vS9, dV;

    if (config.modulation == SSB_LSB || config.modulation == SSB_USB) {
        vS1 = S_STRENGTH_S1;
        vS9 = S_STRENGTH_S9;
        dV = S_STRENGTH_V_DB;
    } else {
        vS1 = S_STRENGTH_S1_LOGAMP;
        vS9 = S_STRENGTH_S9_LOGAMP;
        dV = S_STRENGTH_V_DB_LOGAMP;
    }

    float ss = (float)(((s_strength - vS1) / (vS9 - vS1)) * 8.0) + 1;

    if (ss > 9) {
        // From S9 onwards, each level is +10dB in signal strength
        ss = 9 + (float)((s_strength - vS9) / (10 * dV));
    }

    return ss;
}

float update_s_strength() {

    // s_level = get_s_strength(false, config.modulation == SSB_LSB || config.modulation == SSB_USB ? S_STRENGTH_ADC_CHANNEL : RSSI_ADC_CHANNEL);
    // The new AGC board outputs the conditioned RSSI level at its AFSI output, so we can use the same ADC channel to read it on either mode

    s_level = get_s_strength(false, S_STRENGTH_ADC_CHANNEL);
    return s_level;
}

void check_signal_strength() {

    update_s_strength();

    s_strength_signal.emit(&s_level);

    if (!ISTX && (config.squelch_auto || config.squelch_level > 0)) {

        double squelch_level;

        if (config.squelch_auto) {
            // If squelch is in auto mode, it's level is calculated from the noise floor
            squelch_level = db_to_s_strength(fft_noise_floor_db) + 1.5;
        } else {
            squelch_level = config.squelch_level;
        }

        bool emit = false;
        bool in_squelch = s_level < squelch_level;

        if (in_squelch != last_squelch_test) {
            if (in_squelch) {
                last_activation_trigger_ms = HAL_GetTick();
            } else {
                emit = true; // de-squelch immediately
            }

            last_squelch_test = in_squelch;
        } else if (in_squelch && last_activation_trigger_ms && HAL_GetTick() - last_activation_trigger_ms > SQUELCH_TIMEOUT_MS) {
            emit = true;
            last_activation_trigger_ms = 0;
        }

        if (emit) {
            info.in_squelch = in_squelch;
            info.level = config.squelch_level;
            squelch_signal.emit(&info);
        }
    }
}

void set_squelch(float level) {
    config.squelch_level = level;
    info.in_squelch = false;
    info.level = level;
    squelch_signal.emit(&info);
}

float get_squelch() { return config.squelch_level; }

} // namespace sstrength
