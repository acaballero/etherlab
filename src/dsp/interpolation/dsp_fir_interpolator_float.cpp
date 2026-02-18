//
// Created by Angel Dust on 25/01/2026.
//

#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft_types.h"
#include "dsp/fir_filter.h"
#include "dsp_fir_interpolator_float.h"
#include "dsp/window.h"
#include "handlers.h"
#include <algorithm>
#include <exception>
#include "memory_allocator.h"

template class DspFIRInterpolatorFloatBase<FIR_INTERPOLATOR_BASEBAND_TAPS, float32_t>;

template class DspFIRInterpolatorFloat<FIR_INTERPOLATOR_BASEBAND_TAPS>;

/*
 * Interpolate a sample buffer (I/Q are interleaved)
 * This function is intended to be called for either I or Q channel
 * @param start: 0: Process I samples; 1: Process Q samples
 */
template <int TAPS>
void DspFIRInterpolatorFloat<TAPS>::interpolate(buffer_t<float32_t> &src, buffer_t<float32_t> &dst, uint8_t start, uint8_t n_channels, int start_dst) {

    uint16_t n_samples = src.count / n_channels;
    uint16_t interpolated_block_size = n_samples * this->factor;

    if (start_dst == -1) {
        start_dst = start;
    }

    // Extract the signal from the interleaved buffer
    for (uint16_t i = start, j = 0; j < n_samples; i += n_channels, j++) {
        this->tmp_buff_in[j] = src.p[i];
    }

    arm_fir_interpolate_f32(&this->dsp_fir_interpolate_instance, this->tmp_buff_in, this->tmp_buff_out, n_samples);

    // Write to the final buffer in interleaved format
    for (uint16_t i = start_dst, j = 0; j < interpolated_block_size; i += n_channels, j++) {
        dst.p[i] = this->tmp_buff_out[j];
    }
}

template <int TAPS> void DspFIRInterpolatorFloat<TAPS>::interpolate(buffer_t<complex_t_f32> &src, float32_t *dst_i, float32_t *dst_q) {

    uint16_t n_samples = src.count;

    dsp::unzip_f32((const float32_t *)src.p, this->tmp_buff_in, tmp_buff_in_q, n_samples);

    arm_fir_interpolate_f32(&this->dsp_fir_interpolate_instance, this->tmp_buff_in, dst_i, n_samples);
    arm_fir_interpolate_f32(&dsp_fir_interpolate_instance_q, tmp_buff_in_q, dst_q, n_samples);
}

template <int TAPS> void DspFIRInterpolatorFloat<TAPS>::interpolate(buffer_t<float32_t> &src, buffer_t<float32_t> &dst) {

    uint16_t n_samples = src.count;

    if (src.format == COMPLEX_INTERLEAVED && dst.format == COMPLEX_INTERLEAVED) {

        uint16_t interpolated_block_size = n_samples * this->factor;

        dsp::unzip_f32((const float32_t *)src.p, this->tmp_buff_in, tmp_buff_in_q, n_samples);

        arm_fir_interpolate_f32(&this->dsp_fir_interpolate_instance, this->tmp_buff_in, this->tmp_buff_out, n_samples);
        arm_fir_interpolate_f32(&dsp_fir_interpolate_instance_q, tmp_buff_in_q, tmp_buff_out_q, n_samples);

        dsp::zip_f32(this->tmp_buff_out, tmp_buff_out_q, (float32_t *)dst.p, interpolated_block_size);

    } else if (src.format == COMPLEX_SEQUENTIAL && dst.format == COMPLEX_SEQUENTIAL) {
        interpolate((float32_t *)src.p, (float32_t *)src.p + src.count, (float32_t *)dst.p, (float32_t *)dst.p + src.count * this->factor, src.count);

    } else if (src.format == COMPLEX_SEQUENTIAL && dst.format == COMPLEX_INTERLEAVED) {
        interpolate((float32_t *)src.p, (float32_t *)src.p + src.count, dst, dst.count);

    } else if (src.format == REAL && dst.format == REAL) {
        arm_fir_interpolate_f32(&this->dsp_fir_interpolate_instance, src.p, dst.p, n_samples);

    } else {
        HardFault_Handler();
    }
}

template <int TAPS> void DspFIRInterpolatorFloat<TAPS>::interpolate(float32_t *src_i, float32_t *src_q, float32_t *dst_i, float32_t *dst_q, size_t n_samples) {
    if (this->dsp_fir_interpolate_instance.phaseLength > 100 || dsp_fir_interpolate_instance_q.phaseLength > 100) {
        HardFault_Handler();
    }

    arm_fir_interpolate_f32(&this->dsp_fir_interpolate_instance, src_i, dst_i, n_samples);
    arm_fir_interpolate_f32(&dsp_fir_interpolate_instance_q, src_q, dst_q, n_samples);
}

template <int TAPS> void DspFIRInterpolatorFloat<TAPS>::interpolate(float32_t *src_i, float32_t *src_q, buffer_t<float32_t> &dst, size_t n_samples) {

    uint16_t interpolated_block_size = n_samples * this->factor;

    arm_fir_interpolate_f32(&this->dsp_fir_interpolate_instance, src_i, this->tmp_buff_out, n_samples);
    arm_fir_interpolate_f32(&dsp_fir_interpolate_instance_q, src_q, tmp_buff_out_q, n_samples);

    dsp::zip_f32(this->tmp_buff_out, tmp_buff_out_q, (float32_t *)dst.p, interpolated_block_size);
}

template <int TAPS, typename T> bool DspFIRInterpolatorFloatBase<TAPS, T>::init() {

    bool b = false;

    if (coeffs == nullptr) {
        coeffs = (float32_t *)CCMMemoryAllocator::alloc(TAPS * sizeof(float32_t));
        state = (float32_t *)CCMMemoryAllocator::alloc(state_size);
    }

    if (type == BPF) {
        b = generate_fir_filter_taps(type, coeffs, TAPS, this->input_rate * this->factor, start_frequency, this->bandwidth);
    } else {
        // For interpolation, the filter operates at the OUTPUT rate (input_rate * factor)
        // and needs to pass frequencies up to input_rate/2 (original Nyquist)
        b = generate_fir_filter_taps(type, coeffs, TAPS, this->input_rate * this->factor, this->bandwidth, 0);
    }

    if (b) {
        arm_status status = arm_fir_interpolate_init_f32(&dsp_fir_interpolate_instance, this->factor, TAPS, coeffs, state, DSP_BLOCK);
        if (status != ARM_MATH_SUCCESS) {
            status::pop_alert(status::ERROR, "Error initializing FIR interpolator. TAPS % factor != 0");
            return false;
        }
    }

    initialized = b;

    return b;
}

template <int TAPS> bool DspFIRInterpolatorFloat<TAPS>::init() {

    LOG("Initializing interpolator | rate in: %d | rate out: %d | factor: %d", this->input_rate, this->input_rate * this->factor, this->factor);
    LOG_RAW(" | taps: %d | bandwidth: %d\n", TAPS, this->bandwidth);

    if (state_q == nullptr) {
        state_q = (float32_t *)CCMMemoryAllocator::alloc(this->state_size);
    }

    bool b = DspFIRInterpolatorFloatBase<TAPS, float32_t>::init();

    if (b) {
        arm_status status = arm_fir_interpolate_init_f32(&dsp_fir_interpolate_instance_q, this->factor, TAPS, this->coeffs, state_q, DSP_BLOCK);
        if (status != ARM_MATH_SUCCESS) {
            status::pop_alert(status::ERROR, "Error initializing FIR interpolator Q channel");
            return false;
        }
    }

    return b;
}

template <int TAPS> bool DspFIRInterpolatorFloat<TAPS>::config(uint32_t input_rate, uint32_t bandwidth, uint16_t factor, uint32_t start_frequency) {

    this->input_rate = input_rate;
    this->bandwidth = bandwidth;
    this->factor = factor;
    this->start_frequency = start_frequency;
    this->type = start_frequency ? BPF : LPF;

    return init();
}

template <int TAPS, typename T> void DspFIRInterpolatorFloatBase<TAPS, T>::set_factor(uint16_t factor) {
    this->factor = factor;
    dsp_fir_interpolate_instance.L = factor;
}

template <int TAPS> void DspFIRInterpolatorFloat<TAPS>::set_factor(uint16_t f) {
    this->factor = f;
    dsp_fir_interpolate_instance.L = f;
    dsp_fir_interpolate_instance_q.L = f;
}

template <int TAPS, typename T> void DspFIRInterpolatorFloatBase<TAPS, T>::clear_state() {
    memset(state, 0, state_size);
}

template <int TAPS> void DspFIRInterpolatorFloat<TAPS>::clear_state() {
    DspFIRInterpolatorFloatBase<TAPS, float32_t>::clear_state();
    memset(state_q, 0, this->state_size);
}

template <int TAPS, typename T> bool DspFIRInterpolatorFloatBase<TAPS, T>::get_initialized() const {
    return initialized;
}
