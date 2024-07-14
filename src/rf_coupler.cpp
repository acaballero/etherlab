//
// Created by Angel Dust on 23/12/2021.
//

#include "rf_coupler.h"
#include "hw/stm32.h"
#include "periodic_task.h"
#include "config.h"
#include <printf.h>
#include <float.h>

namespace rf_coupler {

    const int HIGH_SWR = 3;
    const int MAX_SWR = 20;

    uint16_t cpl_offset = CPL_LOGAMP_OFFSET_MV;

    void calculate_power();

    bool enabled = false;
    periodic_task task(50, calculate_power);
    Signal rf_coupler_signal;
    struct rf_coupler_info info;

    void loop() {
        if (enabled) {
            task.loop();
        }
    }

    void enable() {
        enabled = true;
    }

    void disable() {
        enabled = false;
        info = {0, 0, 0, 0, 0};
        rf_coupler_signal.emit(&info);
    }

    void set_offset(uint16_t offset_mv) {
        cpl_offset = offset_mv;
    }

    uint16_t get_offset() {
        return cpl_offset;
    }

    /*
     * Calculates forward and reflected power
     *
     * @see https://www.markimicrowave.com/assets/appnotes/directivity_and_vswr_measurements.pdf
     */
    void calculate_power() {

        struct rf_coupler_info last_info = info;

        float vref = 0;
        float vfor = 0;

        uint16_t v = GetADCValue(&hadc3, FOWARD_POWER_ADC_CHANNEL, 3);
        vfor = adc_to_mv(v, V_REF, MAX_ADC_VALUE);

        v = GetADCValue(&hadc3, REFLECTED_POWER_ADC_CHANNEL, 3);
        vref = adc_to_mv(v, V_REF, MAX_ADC_VALUE);

        vref *= SWR_LOGAMP_FWD_REV_BALANCE;

        // IIR exponential filter
        // Transients and different time constants in the output of the forward and reverse power detectors
        // can led to wrong SWR values. A trade-off between accuracy and response speed has to be found.
        info.v_for = (info.v_for - (0.2 * (info.v_for - vfor)));
        info.v_ref = (info.v_ref - (0.2 * (info.v_ref - vref)));

        //if (v_ref<5) v_ref=0; // Below 5mV at the detector, SWR measurements are too inaccurate to be accounted for

        // Calculate forward power into 50Ohm based in the coupling of the SWR bridge
        if (info.v_ref > info.v_for)
            info.v_ref = info.v_for; // vref should be less or equal vfor (if not, it may be that the directivity of the coupler is really bad or an issue with the ADC's readings)

        // CURVE FITTING (diode detector version)
        // Note that the coefficients are calculated with mV (so the *1000 appearance
        /**
        info.p_for = pow(10, (SWR_FITTING_COEF_A * pow(info.v_for * 1000 - SWR_FITTING_COEF_B, SWR_FITTING_COEF_C)) / 10) /
                     1000; // in watts

        info.p_ref = info.v_ref < 0.03 ? 0 :
                     pow(10, (SWR_FITTING_COEF_A * pow(info.v_ref * 1000 - SWR_FITTING_COEF_B, SWR_FITTING_COEF_C)) / 10) /
                     1000; // in watts
        **/

        // Voltage is a linear function of power V = LOG_AMP_OFFSET + Power_dbm * SWR_LOGAMP_SLOPE_MV mV (logarithmic amplifier version)
        // Pdbm=V-Offset/SWR_LOGAMP_SLOPE_MV
        info.p_for_dbm = info.v_for * 1000 < CPL_LOGAMP_MIN_MV ? -FLT_MAX : (info.v_for * 1000 - cpl_offset) / CPL_LOGAMP_SLOPE_MV;

        info.p_ref_dbm = info.v_ref * 1000 < CPL_LOGAMP_MIN_MV ? -FLT_MAX : (info.v_ref * 1000 - cpl_offset) / CPL_LOGAMP_SLOPE_MV;

        float p_for = toWatts(info.p_for_dbm);
        float p_ref = toWatts(info.p_ref_dbm);

        if (p_for>0) {
            // Peak voltages
            float sqrt_pr_over_pf = sqrt(p_ref / p_for);

            // SWR is 0 (minimum) if no significant power is measured
            info.swr = p_for < 0.0001 ? 0 : (info.v_for > info.v_ref ? (1 + sqrt_pr_over_pf) / (1 - sqrt_pr_over_pf) : FLT_MAX);

        } else {
            // Undefined
            info.swr = 0;
        }

        //printf("%ld; %.4f; %.4f; %.4f; %.4f; %.4f\n", HAL_GetTick(), vfor, vref, info.v_for, info.v_ref, info.swr);

        if (last_info != info) {
            rf_coupler_signal.emit(&info);
            last_info = info;
        }
    }

    float toWatts(float dbm) {
        return dbm == -FLT_MAX ? 0 : pow(10, ((dbm - 30.0) / 10.0));
    }
}