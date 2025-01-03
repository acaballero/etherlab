//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_TYPES_H
#define TRX_FRONTEND_TYPES_H

#include <stdio.h>
#include <string.h>
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
#define FREQ_MEM_NAME_SIZE 10
#define FREQ_MEM_SIZE 50

struct st_freq_mem {
    unsigned long freq;
    MODULATION_MODE mode;
    char name[FREQ_MEM_NAME_SIZE + 1] = "          "; // Must be allocated beforehand or the menu won't let increase it's size beyond the NULL char
    // Copy
    st_freq_mem &operator=(st_freq_mem &o) {
        strncpy(name, o.name, FREQ_MEM_NAME_SIZE);
        mode = o.mode;
        freq = o.freq;
        return o;
    }
};

// Radio status info
struct st_radio_status {
    float squelch_level;
    bool tx;

    bool operator==(const st_radio_status &st) const { return squelch_level == st.squelch_level && tx == st.tx; }
};

// Status bar info
struct st_status {
    MODULATION_MODE modulation;
    bool tx;
    radio::BAND band;
    radio::BAND filter;
    radio::IF_FILTER if_filter;
    radio::FRONTEND_PATH frontend_path;
    bool agc;
    unsigned long f_carrier;

    bool operator==(const st_status &st) const {
        return modulation == st.modulation && tx == st.tx && frontend_path == st.frontend_path && band == st.band && agc == st.agc &&
               if_filter == st.if_filter && f_carrier == st.f_carrier && filter == st.filter; // or another approach as above
    }
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

    bool operator==(const st_freqInfo &st) const { return f_carrier == st.f_carrier && f_step == st.f_step && repeater_mode == st.repeater_mode; }
};

// scale info
struct st_scale {

    int min;
    int max;

    bool operator==(const st_scale &st) const { return min == st.min && max == st.max; }
};

#endif // TRX_FRONTEND_TYPES_H
