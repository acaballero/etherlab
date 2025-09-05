//
// Created by Angel Dust on 16/04/2021.
//

#include "dsp_common.h"
#include "dsp/fft/fft.h"
#include "dsp_config.h"
#include "config.h"
#include "arm_math.h"
#include "status.h"
#include <cstddef>

namespace dsp {

// TX gain for the digital domain
int8_t dsp_tx_gain = 0;

bool freq_shift_enabled = true;

// Current maximum sample frequency. It depends on whether we're doing more or less real time processing to the ADC buffer
uint32_t dsp_max_sample_rate = config.fft.max_sample_rate;

Signal dsp_common_params_signal;

const char *dsp_error_names[] = {"NONE", "ERROR", "FILEOPEN", "FILECLOSE", "FILEWRITE", "FILEREAD", "DMAOVERRUN", "FIFOOVERRUN", "FIFOUNDERRUN"};

st_dsp_status *dsp_status;

st_dsp_config dsp_config;
void set_config(dsp::st_dsp_config &c) {
    dsp_config = c;
}

void set_max_sample_freq(bool dsp) {
    // Set the max sample frequency according to the amount of processing we will be doing
    if (!dsp) {
        dsp_max_sample_rate = config.fft.max_sample_rate;
    } else {
        dsp_max_sample_rate = config.fft.dsp_max_sample_rate;
    }
    fft_config(config.fft.span);
}

void set_max_sample_freq(uint32_t rate) {
    dsp_max_sample_rate = min2(config.fft.dsp_max_sample_rate, rate);
    fft_config(config.fft.span);
}

void set_tx_gain_db(int8_t gain_db) {
    dsp_tx_gain = constrain(gain_db, DSP_MIN_TX_GAIN_DB, DSP_MAX_TX_GAIN_DB);
    dsp_status->gain = pow(10.0, (float)dsp_tx_gain / 20.0);
    dsp_common_params_signal.emit(&dsp_status);
}

void enable_frequency_shift(bool b) {
    freq_shift_enabled = b;
}

bool get_freq_shift_enabled() {
    return fft_params.n_slices == 1 && !ISANALOG && freq_shift_enabled;
}

void set_agc_enabled(bool v) {
    dsp_config.agc_enabled = v;
}

bool get_agc_enabled() {
    return dsp_config.agc_enabled;
}

int32_t get_frequency_shift(uint32_t sample_rate) {
#if DSP_FS4_SHIFT

    int factor = 1;
    if (fft_params.decimation_factor > 1) {
        // Shifting the frequency in hardware helps with some Zero-IF issues, but wastes our LPF bandwidth.
        // To minimise the shift (but still match it with FS/4), we can apply it at a particular decimation
        // state where the sample frequency has already been reduced
        // factor = 2;
    }

    return ((sample_rate ? sample_rate : fft_params.sample_freq) / 4) / factor;

#else
    return 0;
#endif
}

void s16_to_q15(const adc_type *src, q15_t *dst, size_t size) {

    for (size_t i = 0; i < size; i += 2) {
        int32_t packed = *__SIMD32(src)++;

        int16_t s0 = (int16_t)(packed & 0xFFFF);
        int16_t s1 = (int16_t)((packed >> 16) & 0xFFFF);

        // Shift to range
        int32_t q0 = ((s0 - 2048) << 3) & 0xFFFF;
        int32_t q1 = ((s1 - 2048) << 3) & 0xFFFF;

        // Pack back into 32-bit result
        *__SIMD32(dst)++ = (q0 << 16) | q1;
    }
}

adc_type s16_to_f32_and_max_s16(const adc_type *src, float32_t *dst, size_t size) {
    adc_type max;
    size_t i = 0;
    adc_type v1, v2, v3, v4;

    // Unrolling for optimization (not needed when -O3 is used, but nice to have for -Og)
    for (; i + 3 < size; i += 4) {
        v1 = src[i];
        v2 = src[i + 1];
        v3 = src[i + 2];
        v4 = src[i + 3];
        dst[i] = (float32_t)v1;
        dst[i + 1] = (float32_t)v2;
        dst[i + 2] = (float32_t)v3;
        dst[i + 3] = (float32_t)v4;
        if (v1 > max) {
            max = v1;
        }
        if (v2 > max) {
            max = v2;
        }
        if (v3 > max) {
            max = v3;
        }
        if (v4 > max) {
            max = v4;
        }
    }

    // Handle remainder
    for (; i < size; i++) {
        dst[i] = (float32_t)src[i];
        if (src[i] > max) {
            max = src[i];
        }
    }

    return max;
}

void s16_to_f32(const adc_type *src, float32_t *dst, size_t size) {
    size_t i = 0;

    // Unrolling for optimization (not needed when -O3 is used, but nice to have for -Og)
    for (; i + 3 < size; i += 4) {
        dst[i] = (float32_t)src[i];
        dst[i + 1] = (float32_t)src[i + 1];
        dst[i + 2] = (float32_t)src[i + 2];
        dst[i + 3] = (float32_t)src[i + 3];
    }

    // Handle remainder
    for (; i < size; i++) {
        dst[i] = (float32_t)src[i];
    }

    // for (size_t i = 0; i < size; i++) {
    //     *(dst++) = *(src++);
    // }
}

void q15_to_s16(const q15_t *src, adc_type *dst, size_t size) {

    for (size_t i = 0; i < size; i += 2) {

        // Load 2 Q15 values at once (packed into a single 32-bit word)
        int32_t q_pair = *__SIMD32(src)++; // [Q1 | Q0]

        // Shift down by 3 bits to scale from Q15 to 12-bit range
        int32_t q0_shifted = (q_pair & 0xFFFF) >> 3;         // Q0
        int32_t q1_shifted = ((q_pair >> 16) >> 3) & 0xFFFF; // Q1

        // Saturate the result to the 12-bit range of int16_t
        // q0_shifted = __SSAT(q0_shifted, 12);  // Saturate to 12-bit
        // q1_shifted = __SSAT(q1_shifted, 12);  // Saturate to 12-bit

        // Store the results into the output array
        *__SIMD32(dst)++ = ((q1_shifted << 16) & 0xFFFF0000) | (q0_shifted & 0x0000FFFF);
    }
}

void f32_to_s16(const float32_t *src, adc_type *dst, size_t size) {
    for (size_t i = 0; i < size; i++) {
        *(dst++) = (adc_type) * (src++);
    }
}

void unzip_c16(const adc_type *__restrict src, adc_type *__restrict dst_i, adc_type *__restrict dst_q, size_t n_samples) {
    // Extract the signal from the interleaved IQ buffer
    for (uint16_t i = 0; i < n_samples; i += 2) {

        // Load 4 interleaved samples (I0,Q0,I1,Q1)

        int32_t in1 = *__SIMD32(src)++; // SIMD32 [Q0 | I0]
        int32_t in2 = *__SIMD32(src)++; // SIMD32 [Q1 | I1]

        // Extract I samples
        int32_t i_pack = __PKHTB(in2, in1, 16); // [I1 | I0]
        int32_t q_pack = __PKHBT(in1, in2, 16); // [Q1 | Q0]

        *__SIMD32(dst_i)++ = i_pack;
        *__SIMD32(dst_q)++ = q_pack;
    }
}

void zip_c16(const adc_type *__restrict src_i, const adc_type *__restrict src_q, adc_type *__restrict dst, size_t n_samples) {
    // Write to the final buffer in interleaved IQ format
    for (uint16_t i = 0; i < n_samples; i += 2) {

        // Load 2 I and 2 Q samples
        int32_t i_pack = *__SIMD32(src_i)++; // [I1 | I0]
        int32_t q_pack = *__SIMD32(src_q)++; // [Q1 | Q0]

        int32_t out1 = __PKHBT(i_pack, q_pack, 16); // [Q0 | I0]
        int32_t out2 = __PKHTB(q_pack, i_pack, 16); // [Q1 | I1]

        // Store interleaved output
        *__SIMD32(dst)++ = out1;
        *__SIMD32(dst)++ = out2;
    }
}

void unzip_f32(const float32_t *src, float32_t *dst_i, float32_t *dst_q, size_t n_samples) {
    const float32_t *src_end = src + (2 * n_samples);

    // Unroll by 4 complex samples (8 floats)
    while (src + 8 <= src_end) {
        *dst_i++ = *src++; // I0
        *dst_q++ = *src++; // Q0
        *dst_i++ = *src++; // I1
        *dst_q++ = *src++; // Q1
        *dst_i++ = *src++; // I2
        *dst_q++ = *src++; // Q2
        *dst_i++ = *src++; // I3
        *dst_q++ = *src++; // Q3
    }

    // Handle remainder
    while (src < src_end) {
        *dst_i++ = *src++;
        *dst_q++ = *src++;
    }
}
void zip_f32(const float32_t *src_i, float32_t *src_q, float32_t *dst, size_t n_samples) {
    const float32_t *si_end = src_i + n_samples;

    // Unroll by 4 complex samples
    while (src_i + 4 <= si_end) {
        *dst++ = *src_i++; // I0
        *dst++ = *src_q++; // Q0
        *dst++ = *src_i++; // I1
        *dst++ = *src_q++; // Q1
        *dst++ = *src_i++; // I2
        *dst++ = *src_q++; // Q2
        *dst++ = *src_i++; // I3
        *dst++ = *src_q++; // Q3
    }

    // Handle remainder
    while (src_i < si_end) {
        *dst++ = *src_i++;
        *dst++ = *src_q++;
    }
}

/*
 * Sample frequency/4 rotation (frequency shift)
 * Expects complex interleaved buffer (
 */

void rotate_fs4_q15(const q15_t *src, q15_t *dst, size_t n_samples) {
    const uint32_t *src32 = (const uint32_t *)src;
    uint32_t *dst32 = (uint32_t *)dst;

    // Rotation state 0,1,2,3 pattern
    uint32_t rot = 0;

    for (uint32_t i = 0; i < n_samples; ++i) {
        uint32_t in = *src32++; //  [Q | I]

        q15_t i_val = (q15_t)(in & 0xFFFF);
        q15_t q_val = (q15_t)(in >> 16);

        q15_t i_rot, q_rot;

        switch (rot) {
            case 0: // z * 1
                i_rot = i_val;
                q_rot = q_val;
                break;
            case 1: // z * j => -Q + jI
                i_rot = -q_val;
                q_rot = i_val;
                break;
            case 2: // z * -1
                i_rot = -i_val;
                q_rot = -q_val;
                break;
            case 3: // z * -j => Q - jI
                i_rot = q_val;
                q_rot = -i_val;
                break;
        }

        // Pack [Q | I]
        *dst32++ = __PKHBT(i_rot, q_rot, 16);

        // state
        rot = (rot + 1) & 0x3;
    }
}

void log_buff(float32_t *buff, int count, const std::string &title, bool newline) {

    if (title.length()) {
        LOG(title.c_str());
        LOG_RAW(" : ");
    }
    for (int i = 0; i < count; i++) {
        LOG_RAW("%.3f,", buff[i]);
    }
    if (newline) {
        LOG_RAW("-300,\n");
    }
}

void log_buff(adc_type *buff, int count, const std::string &title, bool newline) {

    if (title.length()) {
        LOG(title.c_str());
        LOG_RAW(" : ");
    }
    int min = 100000;
    for (int i = 0; i < count; i++) {
        LOG_RAW("%d,", buff[i]);
        if (min > buff[i]) {
            min = buff[i];
        }
    }

    min -= 20;

    if (newline) {
        LOG_RAW("%d,%d,%d,%d,\n", min, min, min, min);
    }
}
/* THIS ROTATION FUNCTION LOSES A LOT OF PRECISSION
#define __SMULBB(x, y) ((int32_t)(((int16_t)((x)&0xFFFF)) * ((int16_t)((y)&0xFFFF))))
#define __SMULBT(x, y) ((int32_t)(((int16_t)((x)&0xFFFF)) * ((int16_t)((y) >> 16))))
#define __SMULTB(x, y) ((int32_t)(((int16_t)((x) >> 16)) * ((int16_t)((y)&0xFFFF))))
#define __SMULTT(x, y) ((int32_t)(((int16_t)((x) >> 16)) * ((int16_t)((y) >> 16))))

void rotate_fs8_q15(const q15_t *src, q15_t *dst, size_t n_samples) {
const uint32_t *src32 = (const uint32_t *)src;
uint32_t *dst32 = (uint32_t *)dst;

static const uint32_t twiddle_fs8_q15[8] = {0x00007FFF, 0x5A825A82, 0x7FFF0000, 0x5A82A57E, 0x80000000, 0xA57EA57E, 0x00008000, 0x5A82A57E};

for (size_t i = 0; i < n_samples; ++i) {
    uint32_t in = *src32++;
    q15_t i_val = (q15_t)(in & 0xFFFF);
    q15_t q_val = (q15_t)(in >> 16);

    uint32_t tw = twiddle_fs8_q15[i & 0x7];
    q15_t tw_re = (q15_t)(tw & 0xFFFF);
    q15_t tw_im = (q15_t)(tw >> 16);

    int32_t out_i = __SMULBB(i_val, tw_re) - __SMULBB(q_val, tw_im);
    int32_t out_q = __SMULBB(i_val, tw_im) + __SMULBB(q_val, tw_re);

    // Rounding and saturation
    q15_t i_rot = (q15_t)__SSAT((out_i + (1 << 14)) >> 15, 16);
    q15_t q_rot = (q15_t)__SSAT((out_q + (1 << 14)) >> 15, 16);

    i_rot = __SSAT(i_rot << 2, 16);
    q_rot = __SSAT(q_rot << 2, 16);

    *dst32++ = __PKHBT(i_rot, q_rot, 16);
}
}
*/

void rotate_fs4_f32(const float32_t *src, float32_t *dst, size_t n_samples) {

    // Rotation state 0,1,2,3 pattern
    uint32_t rot = 0;

    for (uint32_t i = 0; i < n_samples; ++i) {
        float32_t i_val = *src++;
        float32_t q_val = *src++;

        float32_t i_rot, q_rot;

        switch (rot) {
            case 0: // z * 1
                i_rot = i_val;
                q_rot = q_val;
                break;
            case 1: // z * j => -Q + jI
                i_rot = -q_val;
                q_rot = i_val;
                break;
            case 2: // z * -1
                i_rot = -i_val;
                q_rot = -q_val;
                break;
            case 3: // z * -j => Q - jI
                i_rot = q_val;
                q_rot = -i_val;
                break;
        }

        // Pack [Q | I]
        *dst++ = i_rot;
        *dst++ = q_rot;

        // state
        rot = (rot + 1) & 0x3;
    }
}

} // namespace dsp
