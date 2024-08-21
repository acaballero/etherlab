//
// Created by Angel Dust on 29/07/2024.
//

#include "agc.h"
#include "radio.h"
#include "periodic_task.h"
#include "config.h"
#include <hw/stm32.h>

/** NOT REAL AGC. Just automatic overload gain backoff **/

namespace agc {

    void check_agc();

    Signal signal;

    float agc_voltage;
    periodic_task task(250, check_agc);

    bool overload = false;
    uint64_t last_overload_ms = 0;
    uint16_t overload_auto_correction_delay_ms = 500;

    float get_agc(bool filter) {

        // s_level = get_s_strength(false, config.modulation == SSB_LSB || config.modulation == SSB_USB ? S_STRENGTH_ADC_CHANNEL : RSSI_ADC_CHANNEL);
        // The new AGC board outputs the conditioned RSSI level at its AFSI output, so we can use the same ADC channel to read it on either mode

        uint16_t adcv = GetADCValue(&hadc3, AGC_ADC_CHANNEL, 3);

        float v = ((float) adcv / (float) MAX_ADC_VALUE) * (float) V_REF;

        // exponential filter
        float filter_factor = filter ? 0.3 : 0.95;

        agc_voltage = (agc_voltage -
                       (filter_factor * (agc_voltage - v)));

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
        }
    }

    int get_analog_gain() {
        int if_gain = 24; // TODO: Calculate from agc_voltage (note this will require interpolating and lookup tables of gain vs frequency vs agc)
        return if_gain + frontend_gain();
    }

    void check_agc() {
        /**
           * Try to adjust the DSP gain between config.hw.vg[ab] and MIN_VG[AB]_GAIN
           * to prevent ADC overloading
           */
        get_agc(false);

        signal.emit(&agc_voltage);

        // TODO: This class shouldn't be coupled to board_v2.h and it's gain-specific details
        fft_type max_power_at_dsp = fft_peak + get_analog_gain();

        int max_input_dbm = get_max_input_dbm();

        if (!ISTX) {
            IF_GAIN vga = vga_gain;
            IF_GAIN vgb = vgb_gain;
            uint64_t t = HAL_GetTick();

                if (max_power_at_dsp >= max_input_dbm || fft_mag_overload) {

                if (t - last_overload_ms > overload_auto_correction_delay_ms) {
                    if (vga_gain < MIN_VGA_GAIN) {
                        vga = (IF_GAIN) (vga + 1);
                    } else if (vgb_gain < MIN_VGB_GAIN) {
                        vgb = (IF_GAIN) (vgb + 1);
                    }
                }

                overload = max_power_at_dsp >= max_input_dbm;

            } else {

                overload = false;

                if (!fft_mag_overload && max_power_at_dsp < max_input_dbm - 50) {
                    if (vga_gain > config.hw.cmx973_vga) {
                        vga = (IF_GAIN) (vga - 1);
                    } else if (vgb_gain > config.hw.cmx973_vgb) {
                        vgb = (IF_GAIN) (vgb - 1);
                    }
                }
            }

            if (vga != vga_gain || vgb != vgb_gain) {
                if_gain(RF_DIRECTION_RX, vga, vgb);

                if (overload || fft_mag_overload) {
                    last_overload_ms = t;
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

    void loop() {
        task.loop();
    }
}