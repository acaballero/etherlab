//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_CONFIG_H
#define TRX_FRONTEND_CONFIG_H

#include <stdio.h>
#include <cstdint>

#include "hw/stm32_hal.h"
#include "hw/hw_config.h"
#include "dsp/fft/fft_types.h"
#include "dsp/dsp_config.h"
#include "radio.h"
#include "types.h"
#include "rf_coupler.h"
#include "os/periodic_task.h"

#define DEBUG 0
#define DEBUG_MSGS 1

#define TXMODE(mode) (mode == ANALOG_TX || mode == DIGITAL_TX)
#define ISTX (config.mode == ANALOG_TX || config.mode == DIGITAL_TX)
#define ANALOGMODE(mode) (mode == ANALOG_TX || mode == ANALOG_RX)
#define ISANALOG (ANALOGMODE(config.mode))
#define CONFIG_VERSION "337"

namespace configuration {
extern os::periodic_task task;
}

struct st_vfo_config {
    uint32_t freq = 127100000UL;
    uint32_t step = 10000;
    int32_t rit = 0; // Receive incremental tuning offset

    bool operator==(const st_vfo_config &st) const {
        return freq == st.freq && step == st.step && rit == st.rit;
    }
    bool operator!=(const st_vfo_config &st) const {
        return !(*this == st);
    }
};

// DO NOT use a packed structure if memory constraints are not critical. The misalignment has caused
// me trouble when FPU is enabled and operations are done on misaligned struct members, resulting
// in a Hard Fault
typedef struct st_config //__attribute__ ((packed))
{

    char version[4] = CONFIG_VERSION;

    bool debug = false;

    uint8_t power_ctrl = 0; // Power control byte (8 power control lines)

    radio::BAND filter = radio::BAND_AUTO;
    radio::IF_FILTER if_filter = radio::IF_FILTER_AUTO;
    radio::FRONTEND_PATH frontend_path = radio::FRONTEND_PATH_LNA; // LNA enabled
    MODULATION_MODE modulation = AM;
    MODE mode = DIGITAL_RX;
    radio::BAND band = radio::BAND_AUTO;
    bool hpa_enabled = true;
    uint8_t max_power_dbm = 36;

    // Preferred Local oscillator injection side
    LO_INJECTION lo_injection = HIGH_SIDE;

    // Seconds to enter power save mode. 0=OFF
    uint8_t power_save_period_seconds = 0;

    uint32_t f_1st_if = 73000000L;
    uint32_t f_if_fm_tx = 73320000L; // FM modulator IF frequency

    // VFO config
    uint8_t vfo_ix = 0;
    st_vfo_config vfo[2];
    bool memory_mode = false;

    uint32_t f_carrier = 106700000UL;
    uint32_t f_step = 1000;
    uint32_t f_max = 155000000UL;
    uint32_t f_min = 1000UL;

    // Repeater settings
    uint32_t repeater_offset = 125000;
    radio::RPT_MODE repeater_mode = radio::RPT_MODE_OFF;

    // FFT
    st_fft_config fft;

    // DSP
    dsp::st_dsp_config dsp;

    // Output power from the frequency synthesizers
    LO_POWER lo_drive_strength_0 = LO_POWER_HIGH;   // 1st LO
    LO_POWER lo_drive_strength_1 = LO_POWER_MEDIUM; // 2nd nad 3rd LO

    // Directional coupler 0db output voltage (mV)
    uint16_t coupler_0db_mv = CPL_LOGAMP_OFFSET_MV;

    // Frequency correction applied to the LO synthesiser
    int32_t f_correction = (int32_t)-4305;

    // Frequency correction applied to the IF synthesiser
    int32_t if_correction = (int32_t)-1690; // -1690 with the 27MHz xtco (every
                                            // crystal needs its correction)

    bool squelch_auto = false;
    /* Squelch threshold (S units). 0 to disable */
    float squelch_level = 1;

    bool agc_enabled = true;

    bool enable_quadrature = true;

    // Hardware dependant config struct
    ST_HW_CONFIG hw;

    char callsign[7] = "EA4IAB";

} Config;

extern Config config;

#endif // TRX_FRONTEND_CONFIG_H
