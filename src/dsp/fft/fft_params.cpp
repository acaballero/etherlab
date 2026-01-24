
//
// Created by Angel Dust on 06/09/2025.
//

#include "dsp/fft/fft.h"
#include "hw/stm32.h"
#include "hw/stm32f4xx/timers.h"
#include "stdint.h"
#include <cmath>
#include "dsp/dsp_common.h"
#include "Display_afb.h"
#include "config.h"
#include "fft_params.h"
#include "status.h"

namespace fft {

// Structure to hold fft_params dependencies
struct st_fft_params_dependencies {
    uint32_t span;
    uint32_t freq_mult{0};
    uint8_t decimation_factor_max{1};
    uint8_t max_slices{1};
    uint32_t min_sample_rate{0};
    uint32_t dsp_max_sample_rate{0};

    bool operator==(const st_fft_params_dependencies &other) const {
        return span == other.span && freq_mult == other.freq_mult && decimation_factor_max == other.decimation_factor_max && max_slices == other.max_slices &&
               min_sample_rate == other.min_sample_rate && dsp_max_sample_rate == other.dsp_max_sample_rate;
    }

    bool operator!=(const st_fft_params_dependencies &other) const {
        return !(*this == other);
    }
};

// Cache storage
static st_fft_params_dependencies fft_params_dependencies = {0, 0, 1};
static st_fft_params cached_result;

// Calculates FFT parameters from desired span, decimation factor and n_slices
// If visible_span is given and the object parameters allow resolving for it, calculates the start and end bin accordingly
// It is required that either sample_freq or span are set
void st_fft_params::calc(uint32_t visible_span) {

    // LOG("fft params CALC => visible_span: %d | span: %d | sample_freq: %lu | freq_mult: %lu", visible_span, span, sample_freq, freq_mult);
    //  LOG_RAW(" | factor: %d | min_freq: %d | max_freq: %lu\n", decimation_factor, dsp::dsp_min_sample_rate, dsp::dsp_max_sample_rate);

    if (sample_freq == 0) {
        // Calculate sample frequency, taking into account the usable bandwidth of each slice
        sample_freq = span * ((float)decimation_factor / n_slices / USABLE_BW_FACTOR);

    } else {
        // Fixed sample_freq
        span = sample_freq / ((float)decimation_factor / n_slices / USABLE_BW_FACTOR);
    }

    if (freq_mult) {

        uint32_t target_freq = sample_freq;
        // Ceil to multiple of freq_mult
        sample_freq = ((sample_freq + freq_mult - 1) / freq_mult) * freq_mult;

        // Find nearest achievable frequency with the timer
        sample_freq = get_timer_exact_freq(MAX_DSP_DECIMATION_FACTOR, false, ADC_DMA_TIMER_CLOCK_HZ, sample_freq);

        timer_freq_error = sample_freq % freq_mult;
        if (timer_freq_error != 0) {
            uint32_t lower_mult = (sample_freq / freq_mult) * freq_mult;
            uint32_t upper_mult = lower_mult + freq_mult;

            if (lower_mult >= target_freq) {
                sample_freq = lower_mult;
            } else if (upper_mult >= target_freq) {
                sample_freq = upper_mult;
            }
        }

    } else {
        // Set the real exact achievable frequency in the timer
        sample_freq = get_timer_exact_freq(MAX_DSP_DECIMATION_FACTOR, false, ADC_DMA_TIMER_CLOCK_HZ, sample_freq);
    }

    // Resolution bandwidth (per FFT bin)
    rbw = sample_freq / size / decimation_factor;

    if (visible_span && visible_span < span) {
        span = visible_span;
        nbins = span / rbw / n_slices;
    } else {
        nbins = size * USABLE_BW_FACTOR;
    }

    // Bandwidth per slice
    bw = span / 2 / n_slices;

    // Total bins
    total_bins = nbins * n_slices;

    // Pixels per bin
    bin_width_px = (float)total_bins / (float)DISPLAY_X_PIXELS;

    display_rbw = rbw * bin_width_px;

    slice_w_px = nbins / bin_width_px;

    // first bin to show in each slice
    start_bin = (uint8_t)((float)(size - nbins) / 2.0);

    span_if_start = radio::f_dsp_if - (span >> 1U);
    span_f_start = config.vfo[config.vfo_ix].freq - (span >> 1U);
}

bool st_fft_params::valid_sf() {
    return sample_freq >= dsp::dsp_min_sample_rate && sample_freq <= dsp::dsp_max_sample_rate + freq_mult; // allow DSP_SAMPLE_FREQ_MULT headroom
}

bool st_fft_params::valid() {

    // In digital mode, if near-zero tuning is active (tuning to -sample_freq/4), the available bandwidth gets reduced.
    // After shifting up again +SF/4 in software, the lower cutoff of the pre-ADC low-pass filter is brought up by
    // the same amount
    // (_______X____|___________)
    //         ^----FS/4
    //
    // .....(_______X____|______)
    //    ^-----lost
    //
    // With decimation, the bandwidht of interest is smaller and we can afford losing some phisical bandwidth, which
    // is also taken into account here
    // In the end, we need to be sure that the distance from the (shifted) baseband center frequency to the lower cutoff
    // frequency of the filter is at least the bandowidth of interest (after decimation)

    bool valid_bw = true; // ((int32_t)config.fft.bw - (ISANALOG ? 0 : abs(dsp::get_frequency_shift(sample_freq)))) >= (int32_t)bw;
    return valid_sf() && valid_bw;
}

st_fft_params st_fft_params::find(uint32_t span, uint32_t freq_mult) {

    // In DIGITAL_TX mode, the fft sample rate must be a multiple of DSP_AUDIO_SAMPLE_RATE so we can interpolate/decimate by integer factors
    if (!freq_mult) {
        if (config.mode == DIGITAL_TX) {
            freq_mult = DSP_AUDIO_SAMPLE_RATE;
        } else {
            freq_mult = 0;
        }
    }

    // Create cache key with current parameters
    st_fft_params_dependencies current_dependencies = {.span = span,
                                                       .freq_mult = freq_mult,
                                                       .decimation_factor_max = config.fft.max_decimation_factor,
                                                       .max_slices = get_max_slices(),
                                                       .min_sample_rate = dsp::dsp_min_sample_rate,
                                                       .dsp_max_sample_rate = dsp::dsp_max_sample_rate};

    // Check if we can use cached result
    if (current_dependencies == fft_params_dependencies) {
        return cached_result;
    }

    // Cache miss - need to recalculate
    fft_params_dependencies = current_dependencies;

    // Max span check
    uint32_t max_span = current_dependencies.max_slices * DSP_BANDWIDTH * 2;

    if (span > max_span) {
        span = max_span;
    }

    st_fft_params params{.span = span};
    bool found = false;
    st_fft_params best;

    // FOR DEBUG
    // static uint64_t last_t;
    // static uint32_t last_span = 0;
    // uint32_t t = HAL_GetTick();
    // bool log = false;
    // if (t - last_t > 5000 || last_span != span) {
    //     //  log = true;
    //     last_t = t;
    //     last_span = span;
    // }

    // if (log)
    // LOG("\n\n**** Required span: %d\n", span);
    for (int s = 1; s <= get_max_slices(); s++) {
        for (int d = get_max_decimation(); d >= 1; d >>= 1) {
            // for (int d = 1; d <= config.fft.max_decimation_factor; d <<= 1) {

            params.decimation_factor = d;
            params.n_slices = s;
            params.size = FFT_N;
            params.freq_mult = freq_mult;
            params.sample_freq = 0; // calculate
            params.span = span;
            params.calc();

            if (!params.valid_sf()) {
                //  if (log)
                //      LOG("Invalid try: sr: %u\n", params.sample_freq);

                params.sample_freq = constrain(params.sample_freq, dsp::dsp_min_sample_rate, dsp::dsp_max_sample_rate);

                params.calc();

                if (params.span >= span) {
                    // Reduce the visible bins
                    params.calc(span);
                }

                //   if (log) {
                //       LOG("Changed by: sr: %u, span: %d | start_bin: %d | nbins: %d \n", params.sample_freq, params.span, params.start_bin, params.nbins);
                //   }
            }

            if (params.valid()) {
                //   if (log)
                //       LOG("Current is sr: %u | span: %d | delta: %d | width: %.1f \n", params.sample_freq, params.span, params.span - span,
                //       params.bin_width_px);
                // Cost function is:
                // - Bin width in screen pixels: nearest to one so the bins doesn't have to be stretched nor shrink
                // - Decimation factor: the larger, the better SNR (preferred in digital RX), but also slower rates of FFT update
                auto span_delta = abs((int)params.span - (int)span);
                auto best_span_delta = abs((int)best.span - (int)span);
                bool best_delta = span_delta < best_span_delta;
                if (abs(params.timer_freq_error) < abs(best.timer_freq_error) || best_delta || abs(1 - params.bin_width_px) < abs(1 - best.bin_width_px) ||
                    (config.mode == DIGITAL_RX && (params.decimation_factor > best.decimation_factor))) {
                    best = params;
                    //    if (log)
                    //        LOG("Best is sr: %u | span: %d | delta: %d | width: %.1f |", best.sample_freq, best.span, span_delta, params.bin_width_px);
                    //    if (log)
                    //        LOG_RAW(" dec: %d | start_bin: %d | nbins: %d \n", best.decimation_factor, best.start_bin, best.nbins);

                    found = true;
                }
            } else {
                //   if (log)
                //       LOG("Invalid result\n");
            }
        }
    }

    if (!found) {
        params.decimation_factor = 1;
        params.n_slices = 1;
        params.size = FFT_N;
        params.sample_freq = dsp::dsp_min_sample_rate;

        params.calc();
        best = params;

        // TODO: Currently this function must always return a solution, even if this default one
        found = true;
    }

    cached_result = best;
    return best;
}

// FFT parameters
st_fft_params fft_params;

} // namespace fft
