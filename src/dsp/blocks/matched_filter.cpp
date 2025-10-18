#include "matched_filter.hpp"

#include <algorithm>
#include <cmath>

namespace dsp {
namespace matched_filter {

    void MatchedFilter::configure(const tap_t *const taps, const size_t taps_count, const size_t decimation_factor) {
        samples_ = std::make_unique<samples_t>(taps_count);
        taps_reversed_ = std::make_unique<taps_t>(taps_count);
        taps_count_ = taps_count;
        decimation_factor_ = decimation_factor;
        output = 0;
        std::reverse_copy(&taps[0], &taps[taps_count], &taps_reversed_[0]);
    }

    bool MatchedFilter::execute_once(const sample_t input) {
        samples_[taps_count_ - decimation_factor_ + decimation_phase] = input;

        advance_decimation_phase();
        if (is_new_decimation_cycle()) {
            float sr_tr = 0.0f;
            float si_tr = 0.0f;
            float si_ti = 0.0f;
            float sr_ti = 0.0f;
            for (size_t n = 0; n < taps_count_; n++) {
                const auto sample = samples_[n];
                const auto tap = taps_reversed_[n];

                sr_tr += sample.real() * tap.real();
                si_ti += sample.imag() * tap.imag();
                si_tr += sample.imag() * tap.real();
                sr_ti += sample.real() * tap.imag();
            }

            // N: complex multiple of samples and taps (conjugate, tap.i negated).
            // P: complex multiply of samples and taps.
            const auto r_n = sr_tr + si_ti;
            const auto r_p = sr_tr - si_ti;
            const auto i_n = si_tr - sr_ti;
            const auto i_p = si_tr + sr_ti;

            const auto mag_n = std::sqrt(r_n * r_n + i_n * i_n);
            const auto mag_p = std::sqrt(r_p * r_p + i_p * i_p);
            const auto diff = mag_p - mag_n;
            output = diff;

            shift_by_decimation_factor();
            return true;
        } else {
            return false;
        }
    }

    void MatchedFilter::shift_by_decimation_factor() {
        const sample_t *s = &samples_[decimation_factor_];
        sample_t *t = &samples_[0];

        const size_t unroll_factor = 4;
        size_t shift_count = (taps_count_ - decimation_factor_) / unroll_factor;
        while (shift_count > 0) {
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            *t++ = *s++;
            shift_count--;
        }

        shift_count = (taps_count_ - decimation_factor_) % unroll_factor;
        while (shift_count > 0) {
            *t++ = *s++;
            shift_count--;
        }
    }

} /* namespace matched_filter */
} /* namespace dsp */
