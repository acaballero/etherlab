//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_TYPES_H
#define TRX_FRONTEND_TYPES_H

#include <stdio.h>
#include <cstring>
#include <cstdint>
#include <sys/_stdint.h>
#include "radio.h"
#include "dsp/dsp_common.h"

enum RF_DIRECTION { RF_DIRECTION_RX, RF_DIRECTION_TX };

enum SSB_MODE { SSB_MODE_LSB, SSB_MODE_USB };

enum MODE { ANALOG_RX, ANALOG_TX, DIGITAL_RX, DIGITAL_TX };

enum MODULATION_MODE { SSB_LSB, SSB_USB, FM, WFM, AM, CW, MODULATION_MODE_ALL };

enum DIRECTION { BACKWARDS, STOP, FORWARD };

enum LO_POWER { LO_POWER_LOW, LO_POWER_MEDIUM, LO_POWER_HIGH };

/*
 * Frequency station
 */
#define FREQ_MEM_NAME_SIZE 16
#define FREQ_MEM_SIZE 50

struct st_freq_mem {
    uint16_t group;
    int id = -1;
    uint64_t freq;
    MODULATION_MODE mode;
    char name[FREQ_MEM_NAME_SIZE + 1] = ""; // Must be allocated beforehand or the menu won't let increase it's size beyond the NULL char

    // Default constructor
    st_freq_mem() : group(0), id(0), freq(0), mode(SSB_LSB), name{""} {};

    st_freq_mem(uint16_t g, uint16_t id, unsigned long f, MODULATION_MODE m, const char *n) : group(g), id(id), freq(f), mode(m) {
        strncpy(name, n, FREQ_MEM_NAME_SIZE);
        name[FREQ_MEM_NAME_SIZE] = '\0'; // Ensure null-termination
    }

    st_freq_mem(const st_freq_mem &o) {
        strncpy(name, o.name, FREQ_MEM_NAME_SIZE);
        mode = o.mode;
        freq = o.freq;
        group = o.group;
        id = o.id;
    }

    // Copy
    st_freq_mem &operator=(const st_freq_mem &o) {
        strncpy(name, o.name, FREQ_MEM_NAME_SIZE);
        mode = o.mode;
        freq = o.freq;
        group = o.group;
        id = o.id;
        return *this;
    }
};

// Radio status info
struct st_radio_status {
    float squelch_level;
    bool tx;
    uint8_t vfo_ix;
    int gain;

    bool operator==(const st_radio_status &st) const { return squelch_level == st.squelch_level && tx == st.tx && vfo_ix == st.vfo_ix && gain == st.gain; }
};

// Top bar info
struct st_topBar {
    st_dspStatus *dspState;
    bool mute;

    bool operator==(const st_topBar &st) const { return *dspState == *st.dspState && mute == st.mute; }
};

struct st_freqInfo {
    unsigned long f_carrier;
    unsigned long f_step;
    radio::RPT_MODE repeater_mode;
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

    bool operator==(const st_scale &st) const { return min == st.min && max == st.max; }
};

#endif // TRX_FRONTEND_TYPES_H
