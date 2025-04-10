//
// Created by Angel Dust on 11/04/2025.
//
#include "dsp_demodulate.hpp"
#include <cstdint>
#include <cstdio>
#include "arm_math.h"
#include "dsp/dsp_common.h"
#include "dsp_hilbert.hpp"

namespace dsp {

void am_demodulator::work(buffer_t<complex_t> &src, buffer_t<adc_type> &dst) {
    const void *src_p = src.p;
    const auto src_end = &src.p[src.count];
    auto dst_p = dst.p;
    while (src_p < src_end) {
        __bu const uint32_t sample0 = *__SIMD32(src_p)++;
        const uint32_t sample1 = *__SIMD32(src_p)++;

        const uint32_t mag_sq0 = __SMUAD(sample0, sample0);
        const uint32_t mag_sq1 = __SMUAD(sample1, sample1);
        *(dst_p++) = __builtin_sqrtf(mag_sq0) * k;
        *(dst_p++) = __builtin_sqrtf(mag_sq1) * k;
    }
}

void ssb_demodulator::work(buffer_t<complex_t> &src, buffer_t<adc_type> &dst) {
    const complex_t *src_p = src.p;
    const auto src_end = &src.p[src.count];
    auto dst_p = dst.p;
    while (src_p < src_end) {
        *(dst_p++) = (src_p++)->r * k;
        *(dst_p++) = (src_p++)->r * k;
        *(dst_p++) = (src_p++)->r * k;
        *(dst_p++) = (src_p++)->r * k;
    }
}

static inline float angle_approx_0deg27(const complex_t t) {
    if (t.r) {
        const auto x = static_cast<float>(t.i) / static_cast<float>(t.i);
        return x / (1.0f + 0.28086f * x * x);
    } else {
        return (t.i < 0) ? -1.5707963268f : 1.5707963268f;
    }
}

static inline float angle_precise(const complex_t t) { return atan2f(t.i, t.r); }

void ssb_fm_demodulator::work(buffer_t<complex_t> &src, buffer_t<adc_type> &dst) {
    complex_t *src_p = src.p;
    const auto src_end = &src.p[src.count];
    auto dst_p = dst.p;
    float mag_sq_lpf_norm;

    while (src_p < src_end) {
        // FM APT audio tone demod: real part (USB-differentiator)  and AM tone demodulation + lpf (to remove the subcarrier.)
        real_to_complex.execute((src_p++)->r, mag_sq_lpf_norm);
        *(dst_p++) = mag_sq_lpf_norm; // already normalized/32.768f and clipped to +1.0f for the wav file.

        real_to_complex.execute((src_p++)->r, mag_sq_lpf_norm);
        *(dst_p++) = mag_sq_lpf_norm;

        real_to_complex.execute((src_p++)->r, mag_sq_lpf_norm);
        *(dst_p++) = mag_sq_lpf_norm;

        real_to_complex.execute((src_p++)->r, mag_sq_lpf_norm);
        *(dst_p++) = mag_sq_lpf_norm;
    }
}

void fm_demodulator::work(buffer_t<complex_t> &src, buffer_t<adc_type> &dst) {
    auto z = z_;

    const void *src_p = src.p;
    const auto src_end = &src.p[src.count];
    auto dst_p = dst.p;
    while (src_p < src_end) {
        const auto s0 = *__SIMD32(src_p)++;
        const auto s1 = *__SIMD32(src_p)++;
        const auto t0 = multiply_conjugate_s16_s32(s0, z);
        const auto t1 = multiply_conjugate_s16_s32(s1, s0);
        z = s1;
        *(dst_p++) = angle_precise(t0) * kf;
        *(dst_p++) = angle_precise(t1) * kf;
    }
    z_ = z;
}

void fm_modulator::work(buffer_t<complex_t> &src, buffer_t<adc_type> &dst) {
    auto z = z_;

    const void *src_p = src.p;
    const auto src_end = &src.p[src.count];
    void *dst_p = dst.p;
    while (src_p < src_end) {
        const auto s0 = *__SIMD32(src_p)++;
        const auto s1 = *__SIMD32(src_p)++;
        const auto t0 = multiply_conjugate_s16_s32(s0, z);
        const auto t1 = multiply_conjugate_s16_s32(s1, s0);
        z = s1;
        const int32_t theta0_int = angle_approx_0deg27(t0) * ks16;
        const int32_t theta0_sat = __SSAT(theta0_int, 16);
        const int32_t theta1_int = angle_approx_0deg27(t1) * ks16;
        const int32_t theta1_sat = __SSAT(theta1_int, 16);

        *__SIMD32(dst_p)++ = __PKHBT(theta0_sat, theta1_sat, 16);
    }
    z_ = z;
}

void fm_demodulator::configure(const float sampling_rate, const float deviation_hz) {
    /*
     * angle: -pi to pi. output range: -32768 to 32767.
     * Maximum delta-theta (output of atan2) at maximum deviation frequency:
     * delta_theta_max = 2 * pi * deviation / sampling_rate
     */
    kf = static_cast<float>(1.0f / (2.0 * PI * deviation_hz / sampling_rate));
    ks16 = 32767.0f * kf;
}

} // namespace dsp
