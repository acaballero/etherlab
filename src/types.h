//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_TYPES_H
#define TRX_FRONTEND_TYPES_H

#include <stdio.h>
#include <cstring>
#include <cstdint>
#include "dsp/dsp_common.h"

enum IF_PROCESSING_MODE { ANALOG, DSP };

enum RF_DIRECTION { RF_DIRECTION_RX, RF_DIRECTION_TX };

enum MODE { ANALOG_RX, ANALOG_TX, DIGITAL_RX, DIGITAL_TX, MODE_NONE };

enum MODULATION_MODE { SSB_LSB, SSB_USB, FM, WFM, AM, CW, NONE, MODULATION_MODE_ALL };

enum DIRECTION { BACKWARDS, STOP, FORWARD };

enum LO_POWER { LO_POWER_LOW, LO_POWER_MEDIUM, LO_POWER_HIGH };

enum FREQ_TYPE { STATION, BAND_START, BAND_END, ALL };

// Repeater modes
enum RPT_MODE { RPT_MODE_POSITIVE, RPT_MODE_NEGATIVE, RPT_MODE_OFF };
/*
 * Frequency station
 */
#define FREQ_MEM_NAME_SIZE 32

struct st_freq_mem {

    int32_t id{-1};
    MODULATION_MODE mode;
    FREQ_TYPE type{STATION};
    uint32_t width{0};
    uint64_t freq{0};
    bool repeater{false};
    int32_t offset{0};
    char name[FREQ_MEM_NAME_SIZE + 1] = ""; // Must be allocated beforehand or the menu won't let increase it's size beyond the NULL char

    // Default constructor
    st_freq_mem() : id(0), mode(SSB_LSB), freq(0), name{""} {};

    st_freq_mem(uint16_t id, uint64_t f, MODULATION_MODE m, const char *n) : id(id), mode(m), freq(f) {
        strncpy(name, n, FREQ_MEM_NAME_SIZE);
        name[FREQ_MEM_NAME_SIZE] = '\0'; // Ensure null-termination
    }

    st_freq_mem(const st_freq_mem &o) {
        strncpy(name, o.name, FREQ_MEM_NAME_SIZE);
        mode = o.mode;
        freq = o.freq;
        type = o.type;
        repeater = o.repeater;
        offset = o.offset;
        width = o.width;
        id = o.id;
    }

    // Copy
    st_freq_mem &operator=(const st_freq_mem &o) {
        strncpy(name, o.name, FREQ_MEM_NAME_SIZE);
        mode = o.mode;
        freq = o.freq;
        type = o.type;
        repeater = o.repeater;
        offset = o.offset;
        width = o.width;
        id = o.id;
        return *this;
    }

    // Equals
    bool operator==(const st_freq_mem &o) {
        return id == o.id;
    }
};

// Radio status info
struct st_radio_status {
    float squelch_level;
    bool tx;
    uint8_t vfo_ix;
    int gain;
    bool memory_mode;

    bool operator==(const st_radio_status &st) const {
        return memory_mode == st.memory_mode && squelch_level == st.squelch_level && tx == st.tx && vfo_ix == st.vfo_ix && gain == st.gain;
    }
};

// Top bar info
struct st_topBar {

    bool mute;

    bool operator==(const st_topBar &st) const {
        return mute == st.mute;
    }
};

struct st_freqInfo {
    unsigned long f_carrier;
    unsigned long f_step;
    RPT_MODE repeater_mode;
    uint8_t vfo_ix;
    bool memory_mode;

    bool operator==(const st_freqInfo &st) const {
        return memory_mode == st.memory_mode && f_carrier == st.f_carrier && f_step == st.f_step && repeater_mode == st.repeater_mode && vfo_ix == st.vfo_ix;
    }
};

// scale info
struct st_scale {

    int min;
    int max;

    bool operator==(const st_scale &st) const {
        return min == st.min && max == st.max;
    }
};

// Modulation modes
struct st_modulation_mode {
    MODULATION_MODE modulation;
    bool analog_allowed = false; // Can be demodulated in analog mode?
};

#endif // TRX_FRONTEND_TYPES_H
