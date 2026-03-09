//
// Created by Angel Dust on 11/04/2025.
//
#include "dsp_demodulate.h"
#include <cstdint>
#include <cstdio>
#include "arm_math.h"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft_types.h"
#include "dsp_hilbert.hpp"
#include "status.h"
#include "stm32f4xx_hal_tim.h"

namespace dsp {

static inline float angle_approx_0deg27(const complex_t t) {
    if (t.r) {
        const auto x = static_cast<float>(t.i) / static_cast<float>(t.r); // Fixed: was t.i / t.i
        return x / (1.0f + 0.28086f * x * x);
    } else {
        return (t.i < 0) ? -1.5707963268f : 1.5707963268f;
    }
}

static inline float angle_precise(const complex_t_f32 t) {
    return atan2f(t.i, t.r);
}

static inline complex_t_f32 multiply_conjugate_cs16_cf32(const complex_t a, const complex_t b) {
    /* (a + bj) * (c + dj) = (ac - bd) + (bc + ad)j */
    /* a = i, b = q
     * c = iz1, d = qz1
     */
    const complex_t_f32 result = {(float32_t)a.i * b.r - a.r * b.i, (float32_t)a.i * b.i + a.r * b.r};
    return result;
}

static inline complex_t_f32 multiply_conjugate_cf32_cf32(const complex_t_f32 a, const complex_t_f32 b) {
    /* (a + bj) * (c + dj) = (ac - bd) + (bc + ad)j */
    /* a = i, b = q
     * c = iz1, d = qz1
     */
    const complex_t_f32 result = {a.i * b.r - a.r * b.i, a.i * b.i + a.r * b.r};
    return result;
}

/*
 * Rotate 90 degrees for f/4 frequency shift (to avoid DC-centered demodulation)
 * Static state: No thread safe, and so on...
 */
static inline void rotate_fs4(int16_t &i, int16_t &q) {
    static uint32_t rot_state = 0;
    switch (rot_state++ & 0x3) {
        case 0: // Nothing to be done
            break;
        case 1:
            std::swap(i, q);
            i = -i;
            break;
        case 2:
            i = -i;
            q = -q;
            break;
        case 3:
            std::swap(i, q);
            q = -q;
            break;
    }
}

// ============================================================================
// AM DEMODULATOR
// ============================================================================

void am_demodulator::work(buffer_t<complex_t> &src, adc_type *dst_p) {
    const complex_t *src_p = src.p;
    const auto src_end = &src.p[src.count];

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

void am_demodulator::work(buffer_t<complex_t_f32> &src, float32_t *dst_p) {
    const complex_t_f32 *src_p = src.p;
    const auto src_end = &src.p[src.count];

    while (src_p < src_end) {
        auto sample = src_p++;
        *(dst_p) = __builtin_sqrtf(sample->i * sample->i + sample->r * sample->r);
        dst_p += 2;
    }
}

void am_demodulator::work(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) {
    // Process 4 at a time to maximize FPU pipeline
    for (size_t i = 0; i < count; i += 4) {
        // Prefetch next cache line
        __builtin_prefetch(&src_i[i + 16], 0, 3);
        __builtin_prefetch(&src_q[i + 16], 0, 3);

        // Load in burst
        const float32_t i0 = src_i[i], q0 = src_q[i];
        const float32_t i1 = src_i[i + 1], q1 = src_q[i + 1];
        const float32_t i2 = src_i[i + 2], q2 = src_q[i + 2];
        const float32_t i3 = src_i[i + 3], q3 = src_q[i + 3];

        // Compute in parallel (FPU can pipeline these)
        const float32_t mag0 = __builtin_sqrtf(i0 * i0 + q0 * q0);
        const float32_t mag1 = __builtin_sqrtf(i1 * i1 + q1 * q1);
        const float32_t mag2 = __builtin_sqrtf(i2 * i2 + q2 * q2);
        const float32_t mag3 = __builtin_sqrtf(i3 * i3 + q3 * q3);

        // Store with stride
        dst_p[2 * i] = mag0;
        dst_p[2 * i + 2] = mag1;
        dst_p[2 * i + 4] = mag2;
        dst_p[2 * i + 6] = mag3;
    }
}

void am_demodulator::work_real(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) {
    // Optimized sequential output - much faster than interleaved
    for (size_t i = 0; i < count; i += 4) {
        // Prefetch next cache line
        __builtin_prefetch(&src_i[i + 16], 0, 3);
        __builtin_prefetch(&src_q[i + 16], 0, 3);

        // Load in burst
        const float32_t i0 = src_i[i], q0 = src_q[i];
        const float32_t i1 = src_i[i + 1], q1 = src_q[i + 1];
        const float32_t i2 = src_i[i + 2], q2 = src_q[i + 2];
        const float32_t i3 = src_i[i + 3], q3 = src_q[i + 3];

        // Compute magnitudes in parallel
        const float32_t mag0 = __builtin_sqrtf(i0 * i0 + q0 * q0);
        const float32_t mag1 = __builtin_sqrtf(i1 * i1 + q1 * q1);
        const float32_t mag2 = __builtin_sqrtf(i2 * i2 + q2 * q2);
        const float32_t mag3 = __builtin_sqrtf(i3 * i3 + q3 * q3);

        // Sequential writes - cache friendly!
        dst_p[i] = mag0;
        dst_p[i + 1] = mag1;
        dst_p[i + 2] = mag2;
        dst_p[i + 3] = mag3;
    }
}

// ============================================================================
// SSB DEMODULATOR
// ============================================================================

void ssb_demodulator::work(buffer_t<complex_t> &src, adc_type *dst_p) {
    const complex_t *src_p = src.p;
    const auto src_end = &src.p[src.count];

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

void ssb_demodulator::work(buffer_t<complex_t_f32> &src, float32_t *dst_p) {
    const complex_t_f32 *src_p = src.p;
    const auto src_end = &src.p[src.count];

    while (src_p < src_end) {
        *(dst_p) = (src_p++)->r;
        dst_p += 2;
    }
}

void ssb_demodulator::work(const float32_t *src_i, const float32_t *, float32_t *dst_p, size_t count) {
    for (size_t i = 0; i < count; i += 4) {
        dst_p[2 * i] = src_i[i];
        dst_p[2 * i + 2] = src_i[i + 1];
        dst_p[2 * i + 4] = src_i[i + 2];
        dst_p[2 * i + 6] = src_i[i + 3];
    }
}

void ssb_demodulator::work_real(const float32_t *src_i, const float32_t *, float32_t *dst_p, size_t count) {
    // Super efficient - just copy I channel sequentially
    // This could even be optimized to a single memcpy or ARM optimized copy
    for (size_t i = 0; i < count; i += 8) {
        // Unroll by 8 for maximum throughput
        dst_p[i] = src_i[i];
        dst_p[i + 1] = src_i[i + 1];
        dst_p[i + 2] = src_i[i + 2];
        dst_p[i + 3] = src_i[i + 3];
        dst_p[i + 4] = src_i[i + 4];
        dst_p[i + 5] = src_i[i + 5];
        dst_p[i + 6] = src_i[i + 6];
        dst_p[i + 7] = src_i[i + 7];
    }
    // Alternative: arm_copy_f32(src_i, dst_p, count);
}

// ============================================================================
// SSB_FM DEMODULATOR
// ============================================================================

void ssb_fm_demodulator::work(buffer_t<complex_t> &src, adc_type *dst_p) {
    complex_t *src_p = src.p;
    const auto src_end = &src.p[src.count];
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

void ssb_fm_demodulator::work(buffer_t<complex_t_f32> &, float32_t *) {
    status::handleError(status::ST_ERROR, "Not implemented: SOS filters still not implemented");
}

void ssb_fm_demodulator::work(const float32_t *, const float32_t *, float32_t *, size_t) {
    status::handleError(status::ST_ERROR, "Not implemented");
}

void ssb_fm_demodulator::work_real(const float32_t *, const float32_t *, float32_t *, size_t) {
    status::handleError(status::ST_ERROR, "Not implemented");
}

// ============================================================================
// FM DEMODULATOR
// ============================================================================

void fm_demodulator::work(buffer_t<complex_t> &src, adc_type *dst_p) {
    auto z = z_;

    const void *src_p = src.p;
    const auto src_end = &src.p[src.count];

    while (src_p < src_end) {
        const auto s0 = *__SIMD32(src_p)++;
        const auto s1 = *__SIMD32(src_p)++;
        // Note the use of _rep union to multiply I,Q as a packet
        const auto t0 = multiply_conjugate_cs16_cf32((complex_t){._rep = (uint32_t)s0}, (complex_t){._rep = (uint32_t)z});
        const auto t1 = multiply_conjugate_cs16_cf32((complex_t){._rep = (uint32_t)s1}, (complex_t){._rep = (uint32_t)s0});
        z = s1;
        *(dst_p) = angle_precise(t0) * kf;
        dst_p += 2;
        *(dst_p) = angle_precise(t1) * kf;
        dst_p += 2;
    }
    z_ = z;
}

void fm_demodulator::work(buffer_t<complex_t_f32> &src, float32_t *dst_p) {
    auto prev = zcf32_;
    const complex_t_f32 *src_p = src.p;
    const auto src_end = &src.p[src.count];

    while (src_p < src_end) {
        const auto current = *(src_p);
        const auto t0 = multiply_conjugate_cf32_cf32(current, prev);

        prev = current;

        // kf is an improvement for scaling (might be omitted)
        // the angle_precise is slow but necessary for wideband FM. For narrowband, a good aproximation is: Phase difference ~ (i0*q1 - q0*i1) / (i0^2 + q0^2)
        *(dst_p) = angle_precise(t0) * kf * 10.0;

        // Destination assumed to be interleaved complex.
        // Not required if the target stream is an audio DAC. It'd prevent proper loop unrolling in 16 bit types but with f32 my bet (not measured) is it does
        // not make such a difference other than memory prefetch is not feasible if memory is not linearly accessed. To speed this up, a pointer to 32 bit can
        // be used where only half word is written. However, whatever is gained with the prefetch can be lost in bitwise operations which, by the way, are
        // probably already optimized by the compiler
        dst_p += 2;
        src_p++;
    }
    zcf32_ = prev;
}

void fm_demodulator::work(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) {
    float32_t prev_i = prev_i_f32, prev_q = prev_q_f32;

    // Process 2 samples at a time for better pipeline utilization
    for (size_t i = 0; i < count; i += 2) {
        // Sample 0
        const float32_t curr_i0 = src_i[i];
        const float32_t curr_q0 = src_q[i];

        // Cross-correlation: curr * conj(prev)
        const float32_t real0 = curr_i0 * prev_i + curr_q0 * prev_q;
        const float32_t imag0 = curr_q0 * prev_i - curr_i0 * prev_q;

        // Sample 1 (pipeline with sample 0)
        const float32_t curr_i1 = src_i[i + 1];
        const float32_t curr_q1 = src_q[i + 1];

        const float32_t real1 = curr_i1 * curr_i0 + curr_q1 * curr_q0;
        const float32_t imag1 = curr_q1 * curr_i0 - curr_i1 * curr_q0;

        // Compute angles (this is the bottleneck)
        dst_p[2 * i] = atan2f(imag0, real0) * kf * 10.0f;
        dst_p[2 * i + 2] = atan2f(imag1, real1) * kf * 10.0f;

        prev_i = curr_i1;
        prev_q = curr_q1;
    }

    prev_i_f32 = prev_i;
    prev_q_f32 = prev_q;
}

void fm_demodulator::work_real(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) {
    float32_t prev_i = prev_i_f32, prev_q = prev_q_f32;

    // Optimized for sequential output - much better cache performance
    for (size_t i = 0; i < count; i += 2) {
        // Sample 0
        const float32_t curr_i0 = src_i[i];
        const float32_t curr_q0 = src_q[i];

        // Cross-correlation: curr * conj(prev)
        const float32_t real0 = curr_i0 * prev_i + curr_q0 * prev_q;
        const float32_t imag0 = curr_q0 * prev_i - curr_i0 * prev_q;

        // Sample 1 (pipeline with sample 0)
        const float32_t curr_i1 = src_i[i + 1];
        const float32_t curr_q1 = src_q[i + 1];

        const float32_t real1 = curr_i1 * curr_i0 + curr_q1 * curr_q0;
        const float32_t imag1 = curr_q1 * curr_i0 - curr_i1 * curr_q0;

        // Sequential writes - much better for cache and memory bandwidth
        dst_p[i] = atan2f(imag0, real0) * kf * 10.0f;
        dst_p[i + 1] = atan2f(imag1, real1) * kf * 10.0f;

        prev_i = curr_i1;
        prev_q = curr_q1;
    }

    prev_i_f32 = prev_i;
    prev_q_f32 = prev_q;
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
