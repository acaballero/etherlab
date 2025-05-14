//
// Created by Angel Dust on 17/06/2021.
//

#ifndef TRX_FRONTEND_RADIO_H
#define TRX_FRONTEND_RADIO_H

#include "os/periodic_task.h"
#include <stdio.h>
#include "Signal.h"
#include "mixer.h"
#include <cstdint>

namespace radio {

// TODO: Make configurable
#define CW_PITCH_HZ 750

typedef struct {
    unsigned long freq_start;
    unsigned long freq_end;

    /* Filter control bit weight. There are two power lines per filter (to be able to source/sink enough current from the shift register directly) */
    /* This shoud've been done with driving transistors, but I forgot the current limit of a humble CMOS shift register */
    // uint16_t filter_bank_power_line_a;
    // uint16_t filter_bank_power_line_b;

    // Bits 0-3 int the PORT_A of the hmcp02 MCP23017 instance corresponding to the code to activate filters 0 to 5 in the filter bank
    uint16_t filter_bank_code;

    // Required LO injection side.
    LO_INJECTION lo_injection;

    // Whether TX is allowed in the band
    bool tx_enabled;

} st_band;

typedef struct st_filter {

    unsigned long freq; // For anaog filters, the center frequency
    uint32_t bandwidth;
    bool analog_available = false; // Whether the filter exists in analog mode
    uint8_t pin;                   // For analog filters, the output pin of the IO extender module

} st_filter;

enum FREQ_EVENT { BEFORE_UPDATE, AFTER_UPDATE };

struct st_freq_event {
    uint64_t frequency;
    FREQ_EVENT event;
};

enum BAND {
    BAND_70cm,
    BAND_1m,
    BAND_2m,
    AIRBAND,
    BAND_FM,
    BAND_6m,
    BAND_10m,
    BAND_11m,
    BAND_12m,
    BAND_15m,
    BAND_17m,
    BAND_20m,
    BAND_30m,
    BAND_40m,
    BAND_60m,
    BAND_80m,
    BAND_160m,
    BAND_AUTO,
    BAND_ALL,
    BAND_NONE
};

enum IF_FILTER { IF_FILTER_300HZ, IF_FILTER_3KHZ, IF_FILTER_6KHZ, IF_FILTER_9KHZ, IF_FILTER_15KHZ, IF_FILTER_150KHZ, IF_FILTER_AUTO, IF_FILTER_NONE };

enum IF_FILTER_2 { IF_FILTER_2_AUTO, IF_FILTER_2_AUTO_THRU, IF_FILTER_2_NONE };

// Repeater modes
enum RPT_MODE { RPT_MODE_POSITIVE, RPT_MODE_NEGATIVE, RPT_MODE_OFF };

enum FRONTEND_PATH { FRONTEND_PATH_ATT, FRONTEND_PATH_THRU, FRONTEND_PATH_LNA };

extern const char *bandNames[];
extern const char *IFFilterNames[];
extern const char *modulation_names[];
extern const uint32_t modulation_min_bandwidths[];
extern const char *repeaterNames[];
extern BAND filter;
extern IF_FILTER if_filter;
extern const st_filter if_filters[6];
extern const st_band bands[];
extern mixer mixers[];
extern Signal freq_signal;
// Current quadrature mixer LO frequency
extern uint64_t f_iq;
// IF frequency for the DSP board
extern uint64_t f_dsp_if;
// Tuning frequency
extern uint64_t f_last;

#ifdef __cplusplus
extern "C" {
#endif

void change_frequency(int amount);
void set_frequency(uint64_t f);
void change_step(int amount);
uint64_t get_frequency();
uint64_t get_vfo_frequency(uint8_t);
int64_t get_dsp_frequency_shift();
void set_dsp_frequency_shift(int64_t);
int32_t get_rit();
void set_rit(int32_t v);
void update_freq();
BAND find_band(unsigned long);
bool tx_enabled();
void set_band();
void set_vfo(uint8_t);
uint8_t toggle_vfo();
uint8_t get_vfo();
BAND get_band();
IF_FILTER band_if_filter();
extern os::periodic_task task;
bool is_freq_inverted();
bool is_filter_allowed(IF_FILTER);
uint32_t get_bandwidth_hz();

#ifdef __cplusplus
}
#endif
} // namespace radio
#endif // TRX_FRONTEND_RADIO_H
