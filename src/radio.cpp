//
// Created by Angel Dust on 17/06/2021.
//

#include "stdio.h"
#include "radio.h"
#include "config.h"
#include "signal.h"
#include "scanner.h"
#include "periodic_task.h"
#include <stdint.h>

/*
 * Bits 4-7 int the PORT_A of the hmcp01 MCP23017 instance corresponding
 * to the filters 1 to 6 in the filter bank, #6 being the passthough line
 * See kikad schematic 'FilterBank-SP6T' (Control lines V1-V4)
 * IO expander pin      Filter PCB pin     Left switch code pin  Right switch code pin
 * -----------------------------------------------------------------------------------
 *       5                      1                      1                     1
 *       6                      2                      2                     2
 *       7                      3                      3                     -
 *       4                      4                      -                     4
 *
 *
          v1 v2 v3 v4  band
Filter #6: 1  0  0  1  1m
Filter #5: 0  1  0  1  30MHZ low pass filter for HF bands
Filter #4: 0  0  0  1  70cm
Filter #3: 0  0  1  0  Thru
Filter #2: 0  1  1  0  2m
Filter #1: 1  0  1  0  Airband

The LPF bank after the power amp is wired this way:

 LPF Pin            BPF Pin
----------------------------
   V1                 V3
   V2                 V1
   V3                 V4

Via JST connectors in a mini PCB with diodes for the OR'ed pin  (for the time being  ;))

So that:

 BPF bank              Band        LPF bank
-------------------------------------------------------
Filter #6: 1  0  0  1   1m         0  1  1      #1
Filter #4: 0  0  0  1  70cm        0  0  1      #2
Filter #2: 0  1  1  0   2m         1  0  0      #4
Filter #5: 1  0  1  0    X         1  1  0      #3
 */

#define FLT_V1_IO_BIT 5
#define FLT_V2_IO_BIT 6
#define FLT_V3_IO_BIT 7
#define FLT_V4_IO_BIT 4

#define FLT_6_CODE (1 << FLT_V1_IO_BIT) + (1 << FLT_V4_IO_BIT)
#define FLT_5_CODE (1 << FLT_V2_IO_BIT) + (1 << FLT_V4_IO_BIT)
#define FLT_4_CODE (1 << FLT_V4_IO_BIT)
#define FLT_3_CODE (1 << FLT_V3_IO_BIT)
#define FLT_2_CODE (1 << FLT_V2_IO_BIT) + (1 << FLT_V3_IO_BIT)
#define FLT_1_CODE (1 << FLT_V1_IO_BIT) + (1 << FLT_V3_IO_BIT)
#define FILTER_1_CODE

namespace radio {

// Tapped IF frequency going into DSP
unsigned long f_dsp_if = 0;
unsigned long f_iq = 0;
unsigned long f_last;

Signal freq_signal;

mixer mixers[2];

const st_band bands[] = {{420000000, 450000000, FLT_4_CODE, LOW_SIDE, true},
                         {270000000, 295000000, FLT_6_CODE, ANY_SIDE, true},
                         {143000000, 158000000, FLT_2_CODE, HIGH_SIDE, true},
                         {118000000, 137000000, FLT_1_CODE, HIGH_SIDE, false},
                         {85000000, 110000000, FLT_3_CODE, ANY_SIDE, false},
                         // From this band down the injection has to be high side since the ADF4351 can't go below 35 MHz
                         {50000000, 54000000, FLT_3_CODE, HIGH_SIDE, false},
                         {28000000, 29700000, FLT_5_CODE, HIGH_SIDE, false},
                         {26960000, 27990000, FLT_5_CODE, HIGH_SIDE, false},
                         {24890000, 24990000, FLT_5_CODE, HIGH_SIDE, false},
                         {21000000, 21450000, FLT_5_CODE, HIGH_SIDE, false},
                         {18068000, 18168000, FLT_5_CODE, HIGH_SIDE, false},
                         {13100000, 14800000, FLT_5_CODE, HIGH_SIDE, false},
                         {10100000, 10150000, FLT_5_CODE, HIGH_SIDE, false},
                         {6900000, 7900000, FLT_5_CODE, HIGH_SIDE, false},
                         {5351500, 5366500, FLT_5_CODE, HIGH_SIDE, false},
                         {3500000, 3800000, FLT_5_CODE, HIGH_SIDE, false},
                         {1810000, 1850000, FLT_5_CODE, HIGH_SIDE, false},
                         {7000000, 500000000, FLT_5_CODE, ANY_SIDE, false},
                         {7000000, 500000000, FLT_5_CODE, ANY_SIDE, false}};
const st_filter if_filters[] = {
    {9998500, 3, GPIOEXP_IF_FILTER_3KHZ},      // 3Khz
    {10698000, 15, GPIOEXP_IF_FILTER_15KHZ},   // 15Kh
    {10700000, 150, GPIOEXP_IF_FILTER_150KHZ}, // 150Khz
};
const char *bandNames[] = {"70cm", "1m",  "2m",  "AIRB", "WFM", "6m",  "10m",  "11m",  "12m", "15m",
                           "17m",  "20m", "30m", "40m",  "60m", "80m", "160m", "Auto", "None"};
const char *modulationNames[] = {"LSB", "USB", "FM", "WFM", "AM", "CW"};
const char *IFFilterNames[] = {"3k", "15k", "150k", "Auto"};
const char *IFFilter2Names[] = {"Auto", "Passthru"};
const char *repeaterNames[] = {"+", "-", "Off"};
BAND filter = BAND_NONE;
IF_FILTER if_filter = IF_FILTER_NONE;
IF_FILTER_2 if_filter_2 = IF_FILTER_2_AUTO;

void task_loop();

periodic_task task(50, task_loop);

void calculate_freqs() {

    uint16_t if_bw_khz = if_filters[if_filter].bandwidth_khz;
    int offset = 0;

    unsigned long carrier_freq = get_frequency();

    st_band band = bands[find_band(carrier_freq)];

    // The 1st IF is fixed and common to the two mixers

    mixers[1].setIf(if_filters[if_filter].freq);
    mixers[0].setRf(carrier_freq);

    switch (config.modulation) {

        case SSB_LSB:
        case SSB_USB:
        case CW:

            mixers[0].setIf(config.f_1st_if);

            // Set the injection sides at each IF as required by the wanted sideband
            if (band.lo_injection == ANY_SIDE) {
                mixers[0].setLoInjection(config.modulation == SSB_USB ? HIGH_SIDE : LOW_SIDE);
            } else {
                // We must use a particular injection side in the current band, so we're swapping the 2nd IF LO to select the wanted sideband
                mixers[0].setLoInjection(band.lo_injection);
            }

            // If we need to invert the spectrum, the opposite injection side needs to be used for the 2nd conversion
            mixers[1].setLoInjection(config.modulation == SSB_USB ? (LO_INJECTION)(mixers[0].getLoInjection() * -1) : mixers[0].getLoInjection());
            mixers[1].setRf(config.f_1st_if);

            // Apply an offset to put the left sideband onto the filter passband
            offset = (int)(if_bw_khz * 1000 / 2) + (is_freq_inverted() ? 1000 : 500); // +500 to account for the skirt

            mixers[1].setIf(mixers[1].getIf() + offset);

            /*
             * The injection side depends on the wanted sideband because our crystal filter has a steeper transition on the
             * upper frequencies and, therefore, we want the desired sideband to fall into the passband of the filter
             * (as opposed to: use another filter, pull the existing one, move the LO frequency)
             *
             *    LO SIDE          HI-SIDE
             *   INJECTION        INJECTION
             *
             *    / \  / \        / \  / \
             *   /LSB\/USB\      /USB\/LSB\
             * --------------  --------------
             *    / \             / \
             *   /FLT\           /FLT\
             *       ^
             *       \____ LO FREQUENCY
             *
             */

            break;

        case FM:
        case WFM:
        case AM:

            mixers[1].setLoInjection(LOW_SIDE);
            mixers[0].setLoInjection(band.lo_injection == ANY_SIDE ? config.lo_injection
                                                                   : band.lo_injection); // set the injection to whatever is specified in the settings

            mixers[0].setIf(ISTX ? config.f_if_fm_tx : config.f_1st_if);
            mixers[1].setRf(ISTX && (config.modulation == FM || config.modulation == WFM) ? config.f_if_fm_tx : config.f_1st_if);
            break;
    }

    mixers[0].calcLo();
    mixers[1].calcLo();

    // TAPPED IF center frequency going into DSP. In TX mode, it goes directly from the DSP to the 1st mixer
    f_dsp_if = config.mode == DIGITAL_TX ? mixers[0].getIf() : mixers[1].getIf();
}

bool is_freq_inverted() { return mixers[0].getLoInjection() != mixers[1].getLoInjection(); }

void change_step(int amount) {

    if ((mostSignificantDecimal(config.vfo[config.vfo_ix].step)) != 1) {

        // changing step in tune mode should be in powers of 10
        config.vfo[config.vfo_ix].step = 1000;
    } else {

        if (amount > 0) {
            config.vfo[config.vfo_ix].step *= 10 * amount;
        } else {
            config.vfo[config.vfo_ix].step /= 10 * -amount;
        }

        if (config.vfo[config.vfo_ix].step > 1000000UL) {
            config.vfo[config.vfo_ix].step = 10;
        } else if (config.vfo[config.vfo_ix].step <= 0) {
            config.vfo[config.vfo_ix].step = 1000000UL;
        }
    }
}

uint8_t toggle_vfo() {
    set_vfo(get_vfo() == 0 ? 1 : 0);
    return get_vfo();
}

void set_vfo(uint8_t vfo_ix) {
    if (vfo_ix != config.vfo_ix) {
        config.vfo_ix = vfo_ix;
        update_freq();
    }
}

uint8_t get_vfo() { return config.vfo_ix; }

void change_frequency(int amount) {
    set_frequency(config.vfo[config.vfo_ix].freq + amount * config.vfo[config.vfo_ix].step);
    scanner::stop();
}

// This does not change the frequency immediatelly so it cal be called from an IRQhandler.
void set_frequency(uint64_t f) { config.vfo[config.vfo_ix].freq = f; }

uint64_t get_vfo_frequency(uint8_t vfo_ix) {

    uint64_t f = config.vfo[vfo_ix].freq;

    // TODO: Are we sure we don't need repeater offset in digital mode? That's true for DSP capture & replay
    // but not necessarily in case other DSP modes of operation are implemented

    if (ISTX && ISANALOG && config.repeater_mode != radio::RPT_MODE_OFF) {
        // If repeater mode is enabled, carrier frequency is changed accordingly
        int32_t offset = config.repeater_offset;
        if (config.repeater_mode == radio::RPT_MODE_NEGATIVE) {
            offset *= -1;
        }
        f += offset;
    }

    return f;
}

uint64_t get_frequency() { return get_vfo_frequency(config.vfo_ix); }

void update_freq() {
    config.vfo[config.vfo_ix].freq = constrain(config.vfo[config.vfo_ix].freq, config.f_min, config.f_max);
    calculate_freqs();

    st_freq_event event{config.vfo[config.vfo_ix].freq, BEFORE_UPDATE};
    freq_signal.emit(&event);

    lo_freq(0, mixers[0].getLo());
    lo_freq(1, mixers[1].getLo());

    // Skip the transient
    HAL_Delay(4);

    // TODO: Do not use pointers to void in callbacks
    event = {config.vfo[config.vfo_ix].freq, AFTER_UPDATE};
    freq_signal.emit(&event);

    radio::f_last = config.vfo[config.vfo_ix].freq;
}

int get_pll_multiple(__uint64_t f_freq) {

    int m = 0;

    if (f_freq < 6850000) {
        m = 126;
    } else if ((f_freq >= 6850000) && (f_freq < 9500000)) {
        m = 88;
    } else if ((f_freq >= 9500000) && (f_freq < 13600000)) {
        m = 64;
    } else if ((f_freq >= 13600000) && (f_freq < 17500000)) {
        m = 44;
    } else if ((f_freq >= 17500000) && (f_freq < 25000000)) {
        m = 34;
    } else if ((f_freq >= 25000000) && (f_freq < 36000000)) {
        m = 24;
    }
    if ((f_freq >= 36000000) && (f_freq < 45000000)) {
        m = 18;
    } else if ((f_freq >= 45000000) && (f_freq < 60000000)) {
        m = 14;
    } else if ((f_freq >= 60000000) && (f_freq < 80000000)) {
        m = 10;
    } else if ((f_freq >= 80000000) && (f_freq < 100000000)) {
        m = 8;
    } else if ((f_freq >= 100000000) && (f_freq < 146600000)) {
        m = 6;
    } else if ((f_freq >= 150000000) && (f_freq < 220000000)) {
        m = 4;
    }

    return m;
}

/*
 * Search the band corresponding to a frequency
 */
BAND find_band(unsigned long f) {

    int ix = 0;
    BAND band = BAND_ALL;

    while (ix < BAND_ALL && band == BAND_ALL) {

        if (bands[ix].freq_start <= f && bands[ix].freq_end >= f) {
            band = (BAND)ix;
        }

        ix++;
    }
    return band;
}

BAND get_band() { return find_band(config.vfo[config.vfo_ix].freq); }

void set_band() {
    config.f_min = bands[config.band].freq_start;
    config.f_max = bands[config.band].freq_end;

    set_frequency(constrain(get_frequency(), config.f_min, config.f_max));
}

bool tx_enabled() {
    BAND band = find_band(get_frequency());
    return bands[band].tx_enabled;
}

void loop() { task.loop(); }

void task_loop() {

    if (config.vfo[config.vfo_ix].freq != radio::f_last) {

        radio::BAND band = radio::find_band(radio::f_last);

        // In auto band mode, if we get out of band, we change to the next
        if (config.band == radio::BAND_AUTO) {

            if (config.vfo[config.vfo_ix].freq > radio::bands[band].freq_end) {

                if (band > 0) {
                    // TODO: Skip bands that are not allowed
                    // while (!allowedBand(radio::bands[band - 1])) {

                    //}
                    config.vfo[config.vfo_ix].freq = radio::bands[band - 1].freq_start;
                } else {
                    config.vfo[config.vfo_ix].freq = radio::bands[band].freq_end;
                }

            } else if (config.vfo[config.vfo_ix].freq < radio::bands[band].freq_start) {

                if (band < (radio::BAND_AUTO - 1)) {
                    config.vfo[config.vfo_ix].freq = radio::bands[band + 1].freq_end;
                } else {
                    config.vfo[config.vfo_ix].freq = radio::bands[band].freq_start;
                }
            }
        }

        if (config.vfo[config.vfo_ix].freq != radio::f_last) {
            // Still different?
            update_freq();
        }
    }
}
} // namespace radio
