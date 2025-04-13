//
// Created by Angel Dust on 11/04/2025.
//
#include "dsp_demodulate.hpp"
#include <cstdint>
#include <cstdio>
#include <exception>
#include <sys/_stdint.h>
#include "arm_math.h"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft_types.h"
#include "dsp_hilbert.hpp"
#include "status.h"

namespace dsp {

static inline complex_t_f32 multiply_conjugate_s16_s32(const complex_t a, const complex_t b) {
    /* (a + bj) * (c + dj) = (ac - bd) + (bc + ad)j */
    /* a = i, b = q
     * c = iz1, d = qz1
     */
    const complex_t_f32 result = {(float32_t)a.i * b.i + a.r * b.r, (float32_t)a.r * b.i - a.i * b.r};
    return result;
}

void am_demodulator::work(buffer_t<complex_t> &src, buffer_t<adc_type> &dst) {
    const complex_t *src_p = src.p;
    const auto src_end = &src.p[src.count];
    adc_type *dst_p = dst.p;

    while (src_p < src_end) {

        // two consecutive samples optimized sqrt(i²+q²)
        const uint32_t sample0 = *__SIMD32(src_p)++;
        const uint32_t sample1 = *__SIMD32(src_p)++;
        const uint32_t mag_sq0 = __SMUAD(sample0, sample0);
        const uint32_t mag_sq1 = __SMUAD(sample1, sample1);
        *(dst_p) = __builtin_sqrtf(mag_sq0);
        dst_p += 2;
        *(dst_p) = __builtin_sqrtf(mag_sq1);
        dst_p += 2;
    }
}

void ssb_demodulator::work(buffer_t<complex_t> &src, buffer_t<adc_type> &dst) {
    const complex_t *src_p = src.p;
    const auto src_end = &src.p[src.count];
    auto dst_p = dst.p;
    while (src_p < src_end) { // Loop unrolled for pipeline optimization
        *(dst_p) = (src_p++)->r;
        dst_p += 2;
        *(dst_p) = (src_p++)->r;
        dst_p += 2;
        *(dst_p) = (src_p++)->r;
        dst_p += 2;
        *(dst_p) = (src_p++)->r;
        dst_p += 2;
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

static inline float angle_precise(const complex_t_f32 t) { return atan2f(t.i, t.r); }

void ssb_fm_demodulator::work(buffer_t<complex_t> &src, buffer_t<adc_type> &dst) {
    complex_t *src_p = src.p;
    const auto src_end = &src.p[src.count];
    auto dst_p = dst.p;
    float mag_sq_lpf_norm;

    status::handleError(status::ST_ERROR, "Not implemented: SOS filters still not implemented");

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
        // Note the use of _rep union to multiply I,Q as a packet
        const auto t0 = multiply_conjugate_s16_s32((complex_t){._rep = (uint32_t)s0}, (complex_t){._rep = (uint32_t)z});
        const auto t1 = multiply_conjugate_s16_s32((complex_t){._rep = (uint32_t)s1}, (complex_t){._rep = (uint32_t)s0});
        z = s1;
        *(dst_p++) = angle_precise(t0) * kf;
        *(dst_p++) = angle_precise(t1) * kf;
    }
    z_ = z;
}

// void fm_demodulator::work(buffer_t<complex_t> &src, buffer_t<adc_type> &dst) {
//     auto z = z_;

//     const void *src_p = src.p;
//     const auto src_end = &src.p[src.count];
//     void *dst_p = dst.p;
//     while (src_p < src_end) {
//         const auto s0 = *__SIMD32(src_p)++;
//         const auto s1 = *__SIMD32(src_p)++;
//         const auto t0 = multiply_conjugate_s16_s32(s0, z);
//         const auto t1 = multiply_conjugate_s16_s32(s1, s0);
//         z = s1;
//         const int32_t theta0_int = angle_approx_0deg27(t0) * ks16;
//         const int32_t theta0_sat = __SSAT(theta0_int, 16);
//         const int32_t theta1_int = angle_approx_0deg27(t1) * ks16;
//         const int32_t theta1_sat = __SSAT(theta1_int, 16);

//         *__SIMD32(dst_p)++ = __PKHBT(theta0_sat, theta1_sat, 16);
//     }
//     z_ = z;
// }

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
