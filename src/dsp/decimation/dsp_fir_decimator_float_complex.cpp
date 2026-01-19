//
// Created by Angel Dust on 29/04/2025.
//

#include "MemoryFree.h"
#include "config.h"
#include "dsp/buffer.hpp"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft_types.h"
#include "dsp/fir_filter.h"
#include "dsp_fir_decimator_float_complex.h"
#include "dsp/window.h"
#include <algorithm>
#include <exception>
#include "handlers.h"
#include "printf.h"

template class DspFIRDecimatorFloatComplex<FIR_DECIMATOR_SIGNAL_TAPS>;

template <int TAPS> void DspFIRDecimatorFloatComplex<TAPS>::decimate(buffer_t<float32_t> &src, buffer_t<float32_t> &dst) {

    if (src.format == COMPLEX_SEQUENTIAL && dst.format == COMPLEX_SEQUENTIAL) {
        decimate((float32_t *)src.p, (float32_t *)src.p + src.count, (float32_t *)dst.p, (float32_t *)dst.p + dst.count, dst.count);
    } else if (src.format == REAL && dst.format == REAL) {
        decimate(src.p, dst.p, src.count);
    } else {
        HardFault_Handler();
    }
}

template <int TAPS> void DspFIRDecimatorFloatComplex<TAPS>::decimate(float32_t *src, float32_t *dst, size_t n_samples) {

    // When processing real signals with complex filters, we treat the real signal
    // as a complex signal with zero imaginary part: x[n] = x_real[n] + j*0

    // Apply complex filtering:
    // y_real[n] = FIR(x_real, h_real) - FIR(0, h_imag) = FIR(x_real, h_real)
    // y_imag[n] = FIR(x_real, h_imag) + FIR(0, h_real) = FIR(x_real, h_imag)

    // Process real part: x_real * h_real (this becomes the real output)
    arm_fir_decimate_f32(&fir_xi_hi, src, tmp_buff_i, n_samples);

    // Process imaginary part: x_real * h_imag (this becomes the imaginary output)
    arm_fir_decimate_f32(&fir_xi_hq, src, tmp_buff_q, n_samples);

    // Output only the real part (magnitude preservation)
    for (size_t i = 0; i < n_samples / this->factor; ++i) {
        dst[i] = tmp_buff_i[i]; // Real part only
    }

    // Output the magnitude
    // This gives you the envelope/magnitude of the complex filtered signal
    /*
    for (size_t i = 0; i < n_samples / this->factor; ++i) {
        dst[i] = sqrtf(tmp_buff_i[i] * tmp_buff_i[i] + tmp_buff_q[i] * tmp_buff_q[i]);
    }
    */

    // Output the phase
    // This extracts phase information from the complex filtered signal
    /*
    for (size_t i = 0; i < n_samples / this->factor; ++i) {
        dst[i] = atan2f(tmp_buff_q[i], tmp_buff_i[i]);
    }
    */
}

template <int TAPS> void DspFIRDecimatorFloatComplex<TAPS>::decimate(float32_t *src_i, float32_t *src_q, float32_t *dst_i, float32_t *dst_q, size_t n_samples) {
    arm_fir_decimate_f32(&fir_xi_hi, src_i, tmp_buff_i, n_samples);
    arm_fir_decimate_f32(&fir_xq_hq, src_q, tmp_buff_q, n_samples);

    for (size_t i = 0; i < n_samples / this->factor; ++i) {
        *(dst_i++) = tmp_buff_i[i] - tmp_buff_q[i]; // real
    }

    arm_fir_decimate_f32(&fir_xi_hq, src_i, dst_q, n_samples);
    arm_fir_decimate_f32(&fir_xq_hi, src_q, dst_q, n_samples);

    for (size_t i = 0; i < n_samples / this->factor; ++i) {
        *(dst_q++) = tmp_buff_i[i] - tmp_buff_q[i]; // imag
    }
}

template <int TAPS> bool DspFIRDecimatorFloatComplex<TAPS>::init() {

    bool b = false;

    float32_t coeffs[TAPS * 2];

    if (coeffs_i == nullptr) {

        coeffs_i = (float32_t *)CCMMemoryAllocator::alloc(TAPS * sizeof(float32_t));
        coeffs_q = (float32_t *)CCMMemoryAllocator::alloc(TAPS * sizeof(float32_t));
        state_xi_hi = (float32_t *)CCMMemoryAllocator::alloc(state_size);
        state_xq_hq = (float32_t *)CCMMemoryAllocator::alloc(state_size);
        state_xi_hq = (float32_t *)CCMMemoryAllocator::alloc(state_size);
        state_xq_hi = (float32_t *)CCMMemoryAllocator::alloc(state_size);
        tmp_buff_i = (float32_t *)CCMMemoryAllocator::alloc(DSP_BLOCK * sizeof(float32_t));
        tmp_buff_q = (float32_t *)CCMMemoryAllocator::alloc(DSP_BLOCK * sizeof(float32_t));
    }

    // This generates a complex vector with TAPS*2 length
    b = generate_fir_filter_taps(BPF, coeffs, TAPS, this->input_rate, start_frequency + (this->bandwidth / 2), this->bandwidth / 2);

    // Unzip complex taps
    for (int i = 0; i < TAPS; i++) {
        coeffs_i[i] = coeffs[i * 2 + 0];
        coeffs_q[i] = coeffs[i * 2 + 1];
    }

    // Taps must be reversed to use cmsis decimators
    std::reverse(coeffs_i, coeffs_i + TAPS);
    std::reverse(coeffs_q, coeffs_q + TAPS);

    arm_status status = arm_fir_decimate_init_f32(&fir_xi_hi, TAPS, this->factor, coeffs_i, state_xi_hi, DSP_BLOCK);
    arm_fir_decimate_init_f32(&fir_xq_hq, TAPS, this->factor, coeffs_q, state_xq_hq, DSP_BLOCK);
    arm_fir_decimate_init_f32(&fir_xq_hi, TAPS, this->factor, coeffs_q, state_xq_hi, DSP_BLOCK);
    arm_fir_decimate_init_f32(&fir_xi_hq, TAPS, this->factor, coeffs_i, state_xi_hq, DSP_BLOCK);

    b = b && status == arm_status::ARM_MATH_SUCCESS;

    initialized = b;

    return initialized;
}

template <int TAPS> bool DspFIRDecimatorFloatComplex<TAPS>::config(uint32_t input_rate, uint32_t bandwidth, uint16_t f, uint32_t start_freq) {

    this->input_rate = input_rate;
    this->bandwidth = bandwidth;
    this->factor = f;
    this->start_frequency = start_freq;

    return init();
}

template <int TAPS> void DspFIRDecimatorFloatComplex<TAPS>::clear_state() {
    memset(fir_xi_hi.pState, 0, sizeof(state_xi_hi));
    memset(fir_xq_hq.pState, 0, sizeof(state_xq_hq));
    memset(fir_xq_hi.pState, 0, sizeof(state_xq_hi));
    memset(fir_xi_hq.pState, 0, sizeof(state_xi_hq));
}

template <int TAPS> bool DspFIRDecimatorFloatComplex<TAPS>::get_initialized() const {
    return initialized;
}

template <int TAPS> void DspFIRDecimatorFloatComplex<TAPS>::set_factor(uint16_t f) {
    this->factor = f;
    fir_xi_hi.M = f;
    fir_xq_hq.M = f;
    fir_xi_hq.M = f;
    fir_xq_hq.M = f;
}
