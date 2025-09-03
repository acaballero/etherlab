//
// Created by Angel Dust on 11/12/2022.
//

#include "dc_block.h"
#include "arm_math.h"
#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"
/** Not optimized version **/
inline int16_t DCBlock::filter(const int16_t input) {

    last_y = (float32_t)input - last_x + pole_radius * last_y;
    last_x = (float32_t)input;
    return last_y;
}

inline float32_t DCBlock::filter(const float32_t input) {
    last_y = input - last_x + pole_radius * last_y;
    last_x = input;
    return last_y;
}

void DCBlock::filter(const buffer_t<int16_t> &src, uint8_t n_channels, uint8_t channel_n) {
    for (size_t i = channel_n; i < src.count; i += n_channels) {
        src.p[i] = filter(src.p[i]);
    }
}

void DCBlock::filter(const buffer_t<float32_t> &src, uint8_t n_channels, uint8_t channel_n) {
    for (size_t i = channel_n; i < src.count; i += n_channels) {
        src.p[i] = filter(src.p[i]);
    }
}

/** Highly optimized version
// Force inline and use __restrict for pointer aliasing optimization
__attribute__((always_inline, hot)) inline int16_t DCBlock::filter(const int16_t input) {
    // Use fixed-point arithmetic to avoid float conversions
    // Assuming pole_radius is close to 1.0, use Q15 format
    // Convert pole_radius to Q15 once in constructor: pole_radius_q15 = (int16_t)(pole_radius * 32767)

    // Fast integer implementation
    int32_t input_q15 = (int32_t)input << 15; // Convert to Q30
    int32_t diff = input_q15 - last_x_q15;

    // Multiply by pole_radius in Q15: result in Q30
    int32_t pole_term = ((int64_t)pole_radius_q15 * last_y_q15) >> 15;

    last_y_q15 = (int32_t)(diff + pole_term);
    last_x_q15 = input_q15;

    // Convert back to int16_t
    return (int16_t)(last_y_q15 >> 15);
}

__attribute__((always_inline, hot)) inline float32_t DCBlock::filter(const float32_t input) {
    // Use ARM CMSIS DSP instructions when possible
    float32_t diff = input - last_x;
    last_y = diff + pole_radius * last_y; // Reordered for better pipeline
    last_x = input;
    return last_y;
}

// Optimized batch processing with loop unrolling and vectorization hints
__attribute__((hot)) void DCBlock::filter(const buffer_t<int16_t> &src, uint8_t n_channels, uint8_t channel_n) {
    const size_t count = src.count;
    int16_t *__restrict__ p = src.p; // Restrict pointer for aliasing optimization

    // Cache frequently used values in registers
    int32_t local_last_x = last_x_q15;
    int32_t local_last_y = last_y_q15;
    const int32_t pole_coeff = pole_radius_q15;

    size_t i = channel_n;

    // Process 4 samples at a time when possible (loop unrolling)
    const size_t unroll_limit = count - (count % (4 * n_channels));

    for (; i < unroll_limit; i += 4 * n_channels) {
// Unroll 4 iterations
#pragma GCC unroll 4
        for (int unroll = 0; unroll < 4; ++unroll) {
            size_t idx = i + unroll * n_channels;

            int32_t input_q15 = (int32_t)p[idx] << 15;
            int32_t diff = input_q15 - local_last_x;
            int32_t pole_term = ((int64_t)pole_coeff * local_last_y) >> 15;

            local_last_y = diff + pole_term;
            local_last_x = input_q15;

            p[idx] = (int16_t)(local_last_y >> 15);
        }
    }

    // Handle remaining samples
    for (; i < count; i += n_channels) {
        int32_t input_q15 = (int32_t)p[i] << 15;
        int32_t diff = input_q15 - local_last_x;
        int32_t pole_term = ((int64_t)pole_coeff * local_last_y) >> 15;

        local_last_y = diff + pole_term;
        local_last_x = input_q15;

        p[i] = (int16_t)(local_last_y >> 15);
    }

    // Write back cached values
    last_x_q15 = local_last_x;
    last_y_q15 = local_last_y;
}

__attribute__((hot)) void DCBlock::filter(const buffer_t<float32_t> &src, uint8_t n_channels, uint8_t channel_n) {
    const size_t count = src.count;
    float32_t *__restrict__ p = src.p;

    // Cache state variables in registers
    float32_t local_last_x = last_x;
    float32_t local_last_y = last_y;
    const float32_t pole_coeff = pole_radius;

    size_t i = channel_n;

// Use ARM NEON SIMD when processing multiple channels
#ifdef ARM_MATH_NEON
    if (n_channels >= 4 && count >= 4) {
        // SIMD implementation for 4 channels at once
        float32x4_t last_x_vec = vdupq_n_f32(local_last_x);
        float32x4_t last_y_vec = vdupq_n_f32(local_last_y);
        float32x4_t pole_vec = vdupq_n_f32(pole_coeff);

        for (; i + 3 < count; i += 4) {
            float32x4_t input_vec = vld1q_f32(&p[i]);
            float32x4_t diff_vec = vsubq_f32(input_vec, last_x_vec);
            float32x4_t pole_term = vmulq_f32(pole_vec, last_y_vec);
            last_y_vec = vaddq_f32(diff_vec, pole_term);
            last_x_vec = input_vec;
            vst1q_f32(&p[i], last_y_vec);
        }

        // Extract final state from SIMD register (use last lane)
        local_last_x = vgetq_lane_f32(last_x_vec, 3);
        local_last_y = vgetq_lane_f32(last_y_vec, 3);
    }
#endif

    // Scalar processing with loop unrolling for remaining samples
    const size_t unroll_limit = count - (count % (4 * n_channels));

    for (; i < unroll_limit; i += 4 * n_channels) {
#pragma GCC unroll 4
        for (int unroll = 0; unroll < 4; ++unroll) {
            size_t idx = i + unroll * n_channels;
            float32_t input = p[idx];
            float32_t diff = input - local_last_x;
            local_last_y = diff + pole_coeff * local_last_y;
            local_last_x = input;
            p[idx] = local_last_y;
        }
    }

    // Handle remaining samples
    for (; i < count; i += n_channels) {
        float32_t input = p[i];
        float32_t diff = input - local_last_x;
        local_last_y = diff + pole_coeff * local_last_y;
        local_last_x = input;
        p[i] = local_last_y;
    }

    last_x = local_last_x;
    last_y = local_last_y;
}
**/
