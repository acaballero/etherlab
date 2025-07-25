//
// Created by Angel Dust on 29/07/2024.
//

#include "agc.h"
#include "dsp/dsp.h"
#include "dsp/fft/fft.h"
#include "radio.h"
#include "os/periodic_task.h"
#include "config.h"
#include "status.h"
#include <hw/stm32.h>

/** NOT REAL AGC. Just automatic overload gain backoff **/

namespace agc {

void check_agc();

Signal signal;

float agc_voltage;
static constexpr int task_period_ms = 250;
os::periodic_task task(task_period_ms, check_agc);

bool overload = false;
uint64_t last_overload_state_change = 0;

uint16_t overload_auto_correction_delay_ms = task_period_ms * 4;

float get_agc(bool filter) {

    // s_level = get_s_strength(false, config.modulation == SSB_LSB || config.modulation == SSB_USB ? S_STRENGTH_ADC_CHANNEL : RSSI_ADC_CHANNEL);
    // The new AGC board outputs the conditioned RSSI level at its AFSI output, so we can use the same ADC channel to read it on either mode

    uint16_t adcv = GetADCValue(&hadc3, AGC_ADC_CHANNEL, 3);

    float v = ((float)adcv / (float)MAX_ADC_VALUE) * (float)V_REF;

    // exponential filter
    float filter_factor = filter ? 0.3 : 0.95;

    agc_voltage = (agc_voltage - (filter_factor * (agc_voltage - v)));

    return agc_voltage;
}

int frontend_gain() {
    switch (config.frontend_path) {
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

int get_analog_gain() {
    int if_gain = 25; // TODO: Calculate from agc_voltage (note this will require interpolating and lookup tables of gain vs frequency vs agc)
    return if_gain + frontend_gain();
}
void check_agc() {

    get_agc(false);
    signal.emit(&agc_voltage);

    if (ISTX) {
        return;
    }

    const fft_type power_dbm = fft::dbm_peak + get_analog_gain();
    const int max_dbm = get_max_input_dbm();
    const uint64_t t = HAL_GetTick();

    // Constants
    static const uint32_t ATTACK_MS = 10, RELEASE_MS = 1000, ADC_LOCKOUT_MS = 3000;
    static const int HYSTERESIS_DB = 6, HEADROOM_DB = 12;
    static const uint32_t ADC_OVERLOAD_THRESHOLD = 50;

    // State
    static uint32_t adc_overload_count = 0;
    static uint64_t last_adc_reduction = 0;

    // Helper lambda for gain changes
    auto apply_gain_change = [&](IF_GAIN new_vga, IF_GAIN new_vgb, const char *reason) {
        if (new_vga != vga_gain || new_vgb != vgb_gain) {
            LOG("AGC: %d,%d (%.1fdBm, %s)\n", new_vga, new_vgb, power_dbm, reason);
            if_gain(RF_DIRECTION_RX, new_vga, new_vgb);
            last_overload_state_change = t;
            return true;
        }
        return false;
    };

    // Helper lambda for gain adjustment
    auto adjust_gains = [&](bool reduce) -> std::pair<IF_GAIN, IF_GAIN> {
        IF_GAIN vga = vga_gain, vgb = vgb_gain;
        if (reduce) {
            if (vga < MIN_VGA_GAIN) {
                vga = (IF_GAIN)(vga + 1);
            } else if (vgb < MIN_VGB_GAIN) {
                vgb = (IF_GAIN)(vgb + 1);
            }
        } else {
            if (vgb > config.hw.cmx973_vgb) {
                vgb = (IF_GAIN)(vgb - 1);
            } else if (vga > config.hw.cmx973_vga) {
                vga = (IF_GAIN)(vga - 1);
            }
        }
        return {vga, vgb};
    };

    // ADC overload handling - reduce gain enough to prevent future overload
    adc_overload_count = dsp::adc_overload ? adc_overload_count + 1 : 0;

    if (adc_overload_count >= ADC_OVERLOAD_THRESHOLD) {
        // Calculate how much gain reduction is needed to get below safe threshold
        const int target_level = max_dbm - HEADROOM_DB - 6; // Extra 3dB safety margin
        const int excess_db = power_dbm - target_level;

        // Reduce gain by at least the excess amount (in 3dB steps)
        const int steps_needed = (excess_db + 2) / 6; // Round up

        IF_GAIN vga = vga_gain, vgb = vgb_gain;
        for (int i = 0; i < steps_needed && (vga < MIN_VGA_GAIN || vgb < MIN_VGB_GAIN); i++) {
            if (vga < MIN_VGA_GAIN) {
                vga = (IF_GAIN)(vga + 1);
            } else if (vgb < MIN_VGB_GAIN) {
                vgb = (IF_GAIN)(vgb + 1);
            }
        }

        if (apply_gain_change(vga, vgb, "sustained_adc_overload")) {
            last_adc_reduction = t;

            adc_overload_count = 0;
        }
        return;
    }

    // Power-based AGC
    const bool power_overload = power_dbm >= max_dbm;
    const bool power_safe = power_dbm < (max_dbm - HYSTERESIS_DB);

    if (power_overload != overload) {
        last_overload_state_change = t;
        overload = power_overload;
    }

    const uint64_t time_since_change = t - last_overload_state_change;
    const bool adc_lockout = (t - last_adc_reduction) < ADC_LOCKOUT_MS;

    const bool should_reduce = power_overload && (time_since_change >= ATTACK_MS);
    const bool should_increase = !power_overload && power_safe && (power_dbm < max_dbm - HEADROOM_DB) && (time_since_change >= RELEASE_MS) && !adc_lockout;

    if (should_reduce || should_increase) {
        auto [vga, vgb] = adjust_gains(should_reduce);
        apply_gain_change(vga, vgb, should_reduce ? "power_overload" : "increase_headroom");
    }
}

/**
 * Returns the accummulated gain from frontend to ADC
 * @return
 */
int get_gain() {
    int gain = board_gain();
    return gain + get_analog_gain();
}

bool is_overload() {
    return overload;
}

} // namespace agc
