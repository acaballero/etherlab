//
// Created by Angel Dust on 29/07/2024.
//

#include "agc.h"
#include "dsp/dsp.h"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft.h"
#include "main_board.h"
#include "radio.h"
#include "os/periodic_task.h"
#include "config.h"
#include "status.h"
#include <hw/stm32.h>
#include <sys/_stdint.h>

/** NOT REAL AGC. Just automatic overload gain backoff **/

namespace agc {

void check_agc();

Signal signal_agc_voltage;
Signal signal_gain;

float agc_voltage;
static constexpr int task_period_ms = 250;

static constexpr uint32_t AGC_DEFAULT_RELEASE_MS = 500;
static constexpr uint32_t AGC_DEFAULT_ATTACK_MS = 100;

uint32_t release_ms = AGC_DEFAULT_RELEASE_MS;
uint32_t attack_ms = AGC_DEFAULT_ATTACK_MS;

os::periodic_task task(task_period_ms, check_agc);

bool overload = false;
uint64_t last_overload_state_change = 0;

uint16_t overload_auto_correction_delay_ms = task_period_ms * 4;

// Coefficients for curve fitting the measured AGC voltage VS analog gain
// Basic exponential curve y=a+b*e^(-c*x) with the following coefficients

#define AGC_FITTING_COEFF_A -(1.324064f)
#define AGC_FITTING_COEFF_B 297.7007f
#define AGC_FITTING_COEFF_C 1.863459f

void reset() {

    // Start at minimum gain and let it raise from there (agc.cpp)
    if_gain(RF_DIRECTION_RX, MIN_VGA_GAIN, MIN_VGB_GAIN);

    // Set default attack and release
    set_attack_ms();
    set_release_ms();
}

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

int get_analog_gain() {

    if (ISTX) {
        // TODO: Estimate tx path signal gain before DSP
        return 0;
    } else {
        // 28 is a rough estimate of max gain after 1st and 2nd mixers. It does not account for frequency-variable gain or LO power
        int if_gain = max2(0, 28 - round(AGC_FITTING_COEFF_A + (AGC_FITTING_COEFF_B * exp(-AGC_FITTING_COEFF_C * agc_voltage))));
        return if_gain + main_board::get_frontend_gain();
    }
}

void set_release_ms(uint32_t v) {
    release_ms = v ? v : AGC_DEFAULT_RELEASE_MS;
}

void set_attack_ms(uint32_t v) {
    attack_ms = v ? v : AGC_DEFAULT_ATTACK_MS;
}

void check_agc() {

    get_agc(false);
    signal_agc_voltage.emit(&agc_voltage);

    const fft_type power_dbm = fft::dbm_peak + get_analog_gain();
    volatile const int max_dbm = get_max_input_dbm();
    const uint64_t t = HAL_GetTick();

    // Attack and release time should never be shorter that the time it takes for the FFT to process a new snapshot reflcting the new signal strength
    static const uint32_t ADC_LOCKOUT_MS = 3000;
    static const int HEADROOM_DB = 12;
    static const uint32_t ADC_OVERLOAD_THRESHOLD = 100;

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

    if (dsp::dsp_config.agc_enabled && !ISTX) { // Won't change gain if DSP AGC is disabled or while transmitting
        // Power-based AGC
        const bool power_overload = power_dbm >= max_dbm;

        // LOG("dbm:%.1f,max:%d,g:%d", power_dbm, max_dbm, get_gain());
        // LOG_RAW(",p:%.1f,ag:%d\n", fft::dbm_peak, get_analog_gain());
        if (power_overload != overload) {
            last_overload_state_change = t;
            overload = power_overload;
        }

        const uint64_t time_since_change = t - last_overload_state_change;
        const bool adc_lockout = (t - last_adc_reduction) < ADC_LOCKOUT_MS;

        if (dsp::dsp_config.agc_enabled && agc_voltage < 2) {
            // FIXME: If AGC is enabled and its voltage is low, the DSP gain should not be increased even if power overload is not detected here. Sometimes
            // DSP gain is increased causing saturation and attenuation, which locks the digital gain high.
        }

        const bool should_reduce = power_overload && (time_since_change >= attack_ms);
        const bool should_increase = !power_overload && (power_dbm < max_dbm - HEADROOM_DB) && (time_since_change >= release_ms) && !adc_lockout;

        if (should_reduce || should_increase) {
            auto [vga, vgb] = adjust_gains(should_reduce);
            bool b = apply_gain_change(vga, vgb, should_reduce ? "power_overload" : "increased_headroom");
            if (!b) {
                b = main_board::change_frontend_gain(should_increase ? 1 : -1);
            }

            if (b) {
                signal_gain.emit(nullptr);
            }
        }
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
