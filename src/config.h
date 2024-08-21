//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_CONFIG_H
#define TRX_FRONTEND_CONFIG_H

#include <stdio.h>
#include "hw/stm32_hal.h"
#include "hw/hw_config.h"
#include "dsp/fft/fft_types.h"
#include "radio.h"
#include "types.h"
#include "rf_coupler.h"

#define DEBUG 0
#define DEBUG_MSGS 1

#define TXMODE(mode) (mode==ANALOG_TX || mode==DIGITAL_TX)
#define ISTX (config.mode==ANALOG_TX || config.mode==DIGITAL_TX)
#define ISANALOG (config.mode==ANALOG_TX || config.mode==ANALOG_RX)
#define CONFIG_VERSION "312"

// DO NOT use a packed structure if memory constraints are not critical. The misalignment has caused
// me trouble when FPU is enabled and operations are done on misaligned struct members, resulting
// in a Hard Fault
typedef struct st_config //__attribute__ ((packed))
{
    char version[4] = CONFIG_VERSION;

    bool debug = false;

    uint8_t power_ctrl = 0; // Power control byte (8 power control lines)

    uint8_t filter = radio::BAND_AUTO; // Bypass
    radio::IF_FILTER if_filter = radio::IF_FILTER_AUTO;
    radio::FRONTEND_PATH frontend_path = radio::FRONTEND_PATH_LNA; // LNA enabled
    MODULATION_MODE modulation = FM;
    MODE mode;
    radio::BAND band = radio::BAND_AUTO;
    bool hpa_enabled = true;
    uint8_t max_power_dbm = 36;

    // Preferred Local oscillator injection side
    LO_INJECTION lo_injection = HIGH_SIDE;

    unsigned long f_1st_if = 73000000L;
    unsigned long f_if_fm_tx = 73320000L; // FM modulator IF frequency

    // Carrier frequency
    unsigned long f_carrier = 106700000UL;
    unsigned long f_step = 1000;
    unsigned long f_max = 155000000UL;
    unsigned long f_min = 1000UL;

    // Repeater settings
    uint32_t repeater_offset = 125000;
    radio::RPT_MODE repeater_mode = radio::RPT_MODE_OFF;

    // FFT
    st_fft_config fft;

    // Output power from the frequency synthesizers
    LO_POWER lo_drive_strength_0 = LO_POWER_HIGH; // 1st LO
    LO_POWER lo_drive_strength_1 = LO_POWER_MEDIUM; // 2nd nad 3rd LO

    // Directional coupler 0db output voltage (mV)
    uint16_t coupler_0db_mv = CPL_LOGAMP_OFFSET_MV;

    // Frequency correction applied to the LO synthesiser
    int32_t f_correction = (int32_t) -4305;

    // Frequency correction applied to the IF synthesiser
    int32_t if_correction = (int32_t) -1690; // -1690 with the 27MHz xtco (every crystal needs its correction)

    bool squelch_auto = false;
    /* Squelch threshold (S units). 0 to disable */
    float squelch_level = 0;

    bool agc_enabled = false;

    bool enable_quadrature = true;

    // Hardware dependant config struct
    ST_HW_CONFIG hw;

    // Stations memory
    st_freq_mem freqs[FREQ_MEM_SIZE] = {
            {144300000, SSB_LSB, "TEST 1"},
            {144400000, FM,      "TEST 2"},
    };
} Config;

extern Config config;

#endif //TRX_FRONTEND_CONFIG_H
