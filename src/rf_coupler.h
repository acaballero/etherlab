//
// Created by Angel Dust on 23/12/2021.
//

#ifndef TRX_FRONTEND_RF_COUPLER_H
#define TRX_FRONTEND_RF_COUPLER_H

#include "Signal.h"

// Coefficients for the curve fitting the measured voltage(mV) / power (dBm) at the detector
// It follows a square law for low power levels (linear with power) and a linear relation with voltage for power greater than 30dB
// Anyway, with the curve fitting method we can obtain a fairly accurate conversion
// In this case, we use a shifted power curve y=a(x-b)^c with the following coefficients

//#define SWR_FITTING_COEF_A 12.082142
//#define SWR_FITTING_COEF_B -12.862163
//#define SWR_FITTING_COEF_C 0.183896

// Log amplifier parameters
#define CPL_LOGAMP_SLOPE_MV 25
#define CPL_LOGAMP_OFFSET_MV 1370 // Output at 0 dBm (can be overridden by saved settings)
#define CPL_LOGAMP_MIN_MV 400 // Noise floor output voltage of the logamp

// TODO: Use curve fitting to linearize from 450Mhz to 500Mhz where the AD8307 loses 3dB (if using directional coupler v1.1)

// Balance factor between forward and reverse voltages
// Ideally, both voltages should pass through a similar power detection circuit having identical transfer function.
// In reality there's always an imbalance between both branches (e.g. fwd voltage line is also loaded by the ALC)

#define SWR_LOGAMP_FWD_REV_BALANCE 1

namespace rf_coupler {

    extern const int HIGH_SWR;
    extern const int MAX_SWR;

    struct rf_coupler_info {
        float v_for;
        float v_ref;
        float p_for_dbm;
        float p_ref_dbm;
        float swr;

        bool operator==(const rf_coupler_info &st) const {
            return v_for == st.v_for
                   && v_ref == st.v_ref
                   && p_for_dbm == st.p_for_dbm
                   && p_ref_dbm == st.p_ref_dbm
                   && swr == st.swr;
        }

        bool operator!=(const rf_coupler_info &st) const {
            return !(*this == st);
        }
    };

    extern Signal rf_coupler_signal;
    extern struct rf_coupler_info info;
    void enable();
    void disable();
    void loop();
    void set_offset(uint16_t offset_mv);
    uint16_t get_offset();
    float toWatts(float dbm);

}

#endif //TRX_FRONTEND_RF_COUPLER_H
