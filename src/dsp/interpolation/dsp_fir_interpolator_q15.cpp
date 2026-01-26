//
// Created by Angel Dust on 25/01/2026.
//

#include "dsp/buffer.hpp"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/fir_filter.h"
#include "dsp_fir_interpolator_q15.h"
#include "dsp/window.h"
#include "status.h"
#include <cstddef>

template class DspFIRInterpolatorQ15Base<FIR_DECIMATOR_1ST_HALFBAND_TAPS, complex_t>;
template class DspFIRInterpolatorQ15Base<FIR_DECIMATOR_SIGNAL_TAPS, complex_t>;
template class DspFIRInterpolatorQ15<FIR_DECIMATOR_1ST_HALFBAND_TAPS, complex_t>;
template class DspFIRInterpolatorQ15<FIR_DECIMATOR_SIGNAL_TAPS, complex_t>;

template <int TAPS, typename T> void DspFIRInterpolatorQ15<TAPS, T>::interpolate(buffer_t<T> &src, buffer_t<T> &dst) {
    this->interpolate(src, dst, 0, 2);
}

/*
 * Interpolate a sample buffer (I/Q are interleaved)
 * This function is intended to be called for either I or Q channel
 * @param start: 0: Process I samples; 1: Process Q samples
 */
template <int TAPS, typename T> void DspFIRInterpolatorQ15<TAPS, T>::interpolate(buffer_t<T> &src, buffer_t<T> &dst, uint8_t start, uint8_t n_channels) {

    uint16_t n_samples = src.count / n_channels;
    uint16_t interpolated_block_size = src.count * this->factor;

    // Extract the signal from the interleaved buffer
    for (uint16_t i = start, j = 0; j < n_samples; i += n_channels, j++) {
        this->tmp_buff_in[j] = src.p[i];
    }

    arm_fir_interpolate_q15(&this->dsp_fir_interpolate_instance, this->tmp_buff_in, this->tmp_buff_out, n_samples);

    // Write to the final buffer in interleaved format
    for (uint16_t i = start, j = 0; j < interpolated_block_size; i += 2, j++) {
        dst.p[i] = this->tmp_buff_out[j];
    }
}

template <int TAPS> void DspFIRInterpolatorQ15<TAPS, complex_t>::interpolate(buffer_t<complex_t> &src, adc_type *dst_i, adc_type *dst_q) {

    uint16_t n_samples = src.count;

    dsp::unzip_c16((const adc_type *)src.p, this->tmp_buff_in, tmp_buff_in_q, n_samples);

    arm_fir_interpolate_q15(&dsp_fir_interpolate_instance, tmp_buff_in, dst_i, n_samples);
    arm_fir_interpolate_q15(&dsp_fir_interpolate_instance_q, tmp_buff_in_q, dst_q, n_samples);
}

template <int TAPS> void DspFIRInterpolatorQ15<TAPS, complex_t>::interpolate(buffer_t<complex_t> &src, buffer_t<complex_t> &dst) {

    uint16_t n_samples = src.count;
    uint16_t interpolated_block_size = n_samples * this->factor;

    dsp::unzip_c16((const adc_type *)src.p, this->tmp_buff_in, tmp_buff_in_q, n_samples);

    arm_fir_interpolate_q15(&this->dsp_fir_interpolate_instance, this->tmp_buff_in, this->tmp_buff_out, n_samples);
    arm_fir_interpolate_q15(&dsp_fir_interpolate_instance_q, tmp_buff_in_q, tmp_buff_out_q, n_samples);

    dsp::zip_c16(this->tmp_buff_out, tmp_buff_out_q, (adc_type *)dst.p, interpolated_block_size);
}

template <int TAPS>
void DspFIRInterpolatorQ15<TAPS, complex_t>::interpolate(adc_type *src_i, adc_type *src_q, adc_type *dst_i, adc_type *dst_q, size_t n_samples) {
    arm_fir_interpolate_q15(&this->dsp_fir_interpolate_instance, src_i, dst_i, n_samples);
    arm_fir_interpolate_q15(&dsp_fir_interpolate_instance_q, src_q, dst_q, n_samples);
}

template <int TAPS> void DspFIRInterpolatorQ15<TAPS, complex_t>::interpolate(adc_type *src_i, adc_type *src_q, buffer_t<complex_t> &dst, size_t n_samples) {

    uint16_t interpolated_block_size = n_samples * this->factor;

    arm_fir_interpolate_q15(&this->dsp_fir_interpolate_instance, src_i, this->tmp_buff_out, n_samples);
    arm_fir_interpolate_q15(&dsp_fir_interpolate_instance_q, src_q, tmp_buff_out_q, n_samples);

    dsp::zip_c16(this->tmp_buff_out, tmp_buff_out_q, (adc_type *)dst.p, interpolated_block_size);
}

template <int TAPS, typename T> bool DspFIRInterpolatorQ15Base<TAPS, T>::init() {

    float32_t coeffs_f32[TAPS];

    // For interpolation, the filter operates at the OUTPUT rate (input_rate * factor)
    // and needs to pass frequencies up to input_rate/2 (original Nyquist)
    bool b = generate_fir_filter_taps(LPF, coeffs_f32, TAPS, this->input_rate * this->factor, this->bandwidth, 0);

    // Convert float coefficients to Q15 format
    arm_float_to_q15(coeffs_f32, coeffs, TAPS);

    if (b) {
        arm_status status = arm_fir_interpolate_init_q15(&dsp_fir_interpolate_instance, this->factor, TAPS, coeffs, state, DSP_BLOCK);
        if (status != ARM_MATH_SUCCESS) {
            status::pop_alert(status::ERROR, "Error initializing Q15 FIR interpolator");
            return false;
        }
    }

    initialized = b;

    return b;
}

template <int TAPS> bool DspFIRInterpolatorQ15<TAPS, complex_t>::init() {

    bool b = DspFIRInterpolatorQ15Base<TAPS, complex_t>::init();

    if (b) {
        arm_status status = arm_fir_interpolate_init_q15(&dsp_fir_interpolate_instance_q, this->factor, TAPS, this->coeffs, state_q, DSP_BLOCK);
        if (status != ARM_MATH_SUCCESS) {
            status::pop_alert(status::ERROR, "Error initializing Q15 FIR interpolator Q channel");
            return false;
        }
    }

    return b;
}

template <int TAPS, typename T> bool DspFIRInterpolatorQ15Base<TAPS, T>::config(uint32_t input_rate, uint32_t bandwidth, uint16_t factor) {

    this->input_rate = input_rate;
    this->bandwidth = bandwidth;
    this->factor = factor;

    return init();
}

template <int TAPS, typename T> void DspFIRInterpolatorQ15Base<TAPS, T>::set_factor(uint16_t factor) {
    DspInterpolator<T>::set_factor(factor);
    init();
}

template <int TAPS, typename T> bool DspFIRInterpolatorQ15Base<TAPS, T>::get_initialized() const {
    return initialized;
}
