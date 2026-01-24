
//
// Created by Angel Dust on 06/09/2025.
//

#pragma once

#include "stdint.h"
#include <cmath>
#include <stdint.h>

namespace fft {

typedef class st_fft_params {

  public:
    uint32_t span;

    uint16_t size;

    // Resolution bandwidth per bin
    float rbw;

    // Number of usable bins of each FFT
    uint16_t nbins;

    uint16_t total_bins;

    // Usable bandwidth (half the bandwidth, actually) of each slice
    uint32_t bw = 0;

    uint8_t n_slices = 1;

    uint8_t decimation_factor = 0; // 0: not initialized

    uint32_t sample_freq = 0;

    float bin_width_px = MAXFLOAT;

    // Slice width, in screen pixels
    uint16_t slice_w_px = 0;

    // Start bin of each FFT
    uint16_t start_bin = 0;

    // Resolution bandwidth at the display
    float display_rbw = 0;

    // Absolute start frequency of the span
    uint64_t span_f_start = 0;

    // Starting intermediate frequency of the span
    uint64_t span_if_start = 0;

    uint32_t freq_mult = 0;

    // Frequency offset of the nearest achievable frequency with a timer
    // TODO: This entity shouldn't know about timers (but it's convenient)
    int16_t timer_freq_error = 0;

    void calc(uint32_t span = 0);

    bool valid();

    bool valid_sf();

    static st_fft_params find(uint32_t span, uint32_t freq_mult = 0);

} st_fft_params;

extern st_fft_params fft_params;

} // namespace fft
