//
// Improved modulators implementation - Final Version
// Auto-generates filter coefficients using DSPFilters library
//
#include "dsp_modulate.h"
#include "arm_math.h"
#include "status.h"

namespace dsp {

// ============================================================================
// AM MODULATOR
// ============================================================================

void am_modulator::work(const float32_t *audio_in, buffer_t<complex_t_f32> &iq_out) {
    const float32_t carrier_level = 1.0f;
    const size_t count = iq_out.count;

    // Process 4 samples at a time for FPU pipeline
    size_t i = 0;
    for (; i + 3 < count; i += 4) {
        const float32_t a0 = audio_in[i];
        const float32_t a1 = audio_in[i + 1];
        const float32_t a2 = audio_in[i + 2];
        const float32_t a3 = audio_in[i + 3];

        const float32_t mod0 = carrier_level + mod_index * a0;
        const float32_t mod1 = carrier_level + mod_index * a1;
        const float32_t mod2 = carrier_level + mod_index * a2;
        const float32_t mod3 = carrier_level + mod_index * a3;

        // Both I and Q get same signal for AM
        iq_out.p[i].i = mod0;
        iq_out.p[i].r = mod0;
        iq_out.p[i + 1].i = mod1;
        iq_out.p[i + 1].r = mod1;
        iq_out.p[i + 2].i = mod2;
        iq_out.p[i + 2].r = mod2;
        iq_out.p[i + 3].i = mod3;
        iq_out.p[i + 3].r = mod3;
    }

    for (; i < count; i++) {
        const float32_t mod = carrier_level + mod_index * audio_in[i];
        iq_out.p[i].i = mod;
        iq_out.p[i].r = mod;
    }
}

void am_modulator::work(const float32_t *audio_in, float32_t *iq_out_i, float32_t *iq_out_q, size_t count) {
    const float32_t carrier_level = 1.0f;

    size_t i = 0;
    for (; i + 3 < count; i += 4) {
        const float32_t a0 = audio_in[i];
        const float32_t a1 = audio_in[i + 1];
        const float32_t a2 = audio_in[i + 2];
        const float32_t a3 = audio_in[i + 3];

        const float32_t mod0 = carrier_level + mod_index * a0;
        const float32_t mod1 = carrier_level + mod_index * a1;
        const float32_t mod2 = carrier_level + mod_index * a2;
        const float32_t mod3 = carrier_level + mod_index * a3;

        iq_out_i[i] = mod0;
        iq_out_q[i] = mod0;
        iq_out_i[i + 1] = mod1;
        iq_out_q[i + 1] = mod1;
        iq_out_i[i + 2] = mod2;
        iq_out_q[i + 2] = mod2;
        iq_out_i[i + 3] = mod3;
        iq_out_q[i + 3] = mod3;
    }

    for (; i < count; i++) {
        const float32_t mod = carrier_level + mod_index * audio_in[i];
        iq_out_i[i] = mod;
        iq_out_q[i] = mod;
    }
}

// ============================================================================
// DSB MODULATOR
// ============================================================================

void dsb_modulator::work(const float32_t *audio_in, buffer_t<complex_t_f32> &iq_out) {
    const size_t count = iq_out.count;

    size_t i = 0;
    for (; i + 7 < count; i += 8) {
        __builtin_prefetch(&audio_in[i + 16], 0, 3);

        iq_out.p[i].i = audio_in[i];
        iq_out.p[i].r = audio_in[i];
        iq_out.p[i + 1].i = audio_in[i + 1];
        iq_out.p[i + 1].r = audio_in[i + 1];
        iq_out.p[i + 2].i = audio_in[i + 2];
        iq_out.p[i + 2].r = audio_in[i + 2];
        iq_out.p[i + 3].i = audio_in[i + 3];
        iq_out.p[i + 3].r = audio_in[i + 3];
        iq_out.p[i + 4].i = audio_in[i + 4];
        iq_out.p[i + 4].r = audio_in[i + 4];
        iq_out.p[i + 5].i = audio_in[i + 5];
        iq_out.p[i + 5].r = audio_in[i + 5];
        iq_out.p[i + 6].i = audio_in[i + 6];
        iq_out.p[i + 6].r = audio_in[i + 6];
        iq_out.p[i + 7].i = audio_in[i + 7];
        iq_out.p[i + 7].r = audio_in[i + 7];
    }

    for (; i < count; i++) {
        iq_out.p[i].i = audio_in[i];
        iq_out.p[i].r = audio_in[i];
    }
}

void dsb_modulator::work(const float32_t *audio_in, float32_t *iq_out_i, float32_t *iq_out_q, size_t count) {
    // Manual copy with loop unrolling (more efficient than memcpy for small sizes)
    size_t i = 0;
    for (; i + 7 < count; i += 8) {
        __builtin_prefetch(&audio_in[i + 16], 0, 3);

        iq_out_i[i] = audio_in[i];
        iq_out_q[i] = audio_in[i];
        iq_out_i[i + 1] = audio_in[i + 1];
        iq_out_q[i + 1] = audio_in[i + 1];
        iq_out_i[i + 2] = audio_in[i + 2];
        iq_out_q[i + 2] = audio_in[i + 2];
        iq_out_i[i + 3] = audio_in[i + 3];
        iq_out_q[i + 3] = audio_in[i + 3];
        iq_out_i[i + 4] = audio_in[i + 4];
        iq_out_q[i + 4] = audio_in[i + 4];
        iq_out_i[i + 5] = audio_in[i + 5];
        iq_out_q[i + 5] = audio_in[i + 5];
        iq_out_i[i + 6] = audio_in[i + 6];
        iq_out_q[i + 6] = audio_in[i + 6];
        iq_out_i[i + 7] = audio_in[i + 7];
        iq_out_q[i + 7] = audio_in[i + 7];
    }

    for (; i < count; i++) {
        iq_out_i[i] = audio_in[i];
        iq_out_q[i] = audio_in[i];
    }
}
// ============================================================================
// SSB MODULATOR
// ============================================================================

bool ssb_modulator::configure(uint32_t sr, uint32_t bw) {
    // Check if reconfiguration needed
    if (configured && sample_rate == sr && bandwidth == bw) {
        return true;
    }

    sample_rate = sr;
    bandwidth = bw;
    configured = hilbert.configure(sample_rate);

    return configured;
}

void ssb_modulator::work(const float32_t *audio_in, buffer_t<complex_t_f32> &iq_out) {
    const size_t count = iq_out.count;
    float32_t i_sample, q_sample;

    for (size_t i = 0; i < count; i++) {

        // Hilbert transform creates I/Q pair
        hilbert.execute(audio_in[i], i_sample, q_sample);

        // Apply sideband selection
        if (mode == USB) {
            iq_out.p[i].i = i_sample;
            iq_out.p[i].r = q_sample;
        } else { // LSB
            iq_out.p[i].i = i_sample;
            iq_out.p[i].r = -q_sample; // Conjugate for LSB
        }
    }
}

void ssb_modulator::work(const float32_t *audio_in, float32_t *iq_out_i, float32_t *iq_out_q, size_t count) {
    for (size_t i = 0; i < count; i++) {
        float32_t i_sample, q_sample;

        hilbert.execute(audio_in[i], i_sample, q_sample);

        iq_out_i[i] = i_sample;
        iq_out_q[i] = (mode == USB) ? q_sample : -q_sample;
    }
}

// ============================================================================
// FM MODULATOR
// ============================================================================

void fm_modulator::configure(float32_t sr, float32_t dev) {
    sample_rate = sr;
    deviation = dev;

    // Calculate modulation constant
    // φ(t) = 2π * deviation * ∫audio(t)dt
    // Phase increment per sample = 2π * deviation * audio / sample_rate
    kf = (2.0f * PI * deviation) / sample_rate;
}

inline void fm_modulator::get_sin_cos(float32_t phase, float32_t &sin_val, float32_t &cos_val) {
    // Use CMSIS-DSP fast sine/cosine (expects degrees)
    arm_sin_cos_f32(phase * (180.0f / PI), &sin_val, &cos_val);
}

void fm_modulator::work(const float32_t *audio_in, buffer_t<complex_t_f32> &iq_out) {

    const size_t count = iq_out.count;

    for (size_t i = 0; i < count; i++) {
        // Phase modulation
        phase += kf * audio_in[i];

        // Wrap phase to [-π, π] for numerical stability
        while (phase > PI) {
            phase -= 2.0f * PI;
        }
        while (phase < -PI) {
            phase += 2.0f * PI;
        }

        // Generate I/Q samples
        float32_t sin_val, cos_val;
        get_sin_cos(phase, sin_val, cos_val);

        iq_out.p[i].i = cos_val; // I = cos(phase)
        iq_out.p[i].r = sin_val; // Q = sin(phase)
    }
}

void fm_modulator::work(const float32_t *audio_in, float32_t *iq_out_i, float32_t *iq_out_q, size_t count) {

    for (size_t i = 0; i < count; i++) {
        phase += kf * audio_in[i];

        while (phase > PI) {
            phase -= 2.0f * PI;
        }
        while (phase < -PI) {
            phase += 2.0f * PI;
        }

        float32_t sin_val, cos_val;
        get_sin_cos(phase, sin_val, cos_val);

        iq_out_i[i] = cos_val;
        iq_out_q[i] = sin_val;
    }
}

} /* namespace dsp */
