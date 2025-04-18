//
// Created by Angel Dust on 21/04/2021.
//

#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft_types.h"
#include "dsp/firFilter.h"
#include "dsp_fir_decimator_float.h"
#include "dsp/window.h"
#include <exception>

template class DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS, float>;
template class DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS, adc_type>;
template class DspFIRDecimatorFloat<FIR_DECIMATOR_1ST_HALFBAND_TAPS, complex_t_f32>;
template class DspFIRDecimatorFloat<FIR_DECIMATOR_SIGNAL_TAPS, complex_t_f32>;

template <int TAPS, typename T> void DspFIRDecimatorFloat<TAPS, T>::decimate(buffer_t<T> &src, buffer_t<T> &dst) {
    this->decimate(src, dst, 0, 2);
}

/*
 * Decimate a DSP_BLOCK size I/Q sample buffer (I/Q are interleaved)
 * This function is intended to be called either for the I or Q half of the buffer
 * @param Start: 0: Process I samples; 1: Process Q samples
 */

//__attribute__((section(".ccmram")))
template <int TAPS, typename T> void DspFIRDecimatorFloat<TAPS, T>::decimate(buffer_t<T> &src, buffer_t<T> &dst, uint8_t start, uint8_t n_channels) {

    // TODO: Consider skipping the first processed blocks to account for the delay group of the filter

    uint16_t n_samples = src.count / n_channels;
    uint16_t decimated_block_size = n_samples / this->factor; // DMA buffer size (DSP_BLOCK) / decimation factor

    // Extract the signal from the interleaved IQ buffer
    for (uint16_t i = start, j = 0; j < n_samples; i += n_channels, j++) {
        this->tmp_buff_in[j] = src.p[i];
    }

    arm_fir_decimate_f32(&this->dsp_fir_decimate_instance, this->tmp_buff_in, this->tmp_buff_out, n_samples);

    // Write to the final adc_buffer in interleaved IQ format
    for (uint16_t i = start, j = 0; j < decimated_block_size; i += n_channels, j++) {
        dst.p[i] = this->tmp_buff_out[j];
    }
}

template <int TAPS> void DspFIRDecimatorFloat<TAPS, complex_t_f32>::decimate(buffer_t<complex_t_f32> &src, buffer_t<complex_t_f32> &dst) {

    uint16_t n_samples = src.count;
    uint16_t decimated_block_size = n_samples / this->factor; // DMA buffer size (DSP_BLOCK) / decimation factor

    // Extract the signal from the interleaved IQ buffer
    for (uint16_t i = 0; i < n_samples; i++) {
        tmp_buff_in[i] = src.p[i].i;
        tmp_buff_in_q[i] = src.p[i].r;
    }

    arm_fir_decimate_f32(&dsp_fir_decimate_instance, tmp_buff_in, tmp_buff_out, n_samples);
    arm_fir_decimate_f32(&dsp_fir_decimate_instance_q, tmp_buff_in_q, tmp_buff_out_q, n_samples);

    // Write to the final buffer in interleaved IQ format
    for (uint16_t i = 0; i < decimated_block_size; i++) {
        dst.p[i].i = tmp_buff_out[i];
        dst.p[i].r = tmp_buff_out_q[i];
    }
}

template <int TAPS, typename T> bool DspFIRDecimatorFloatBase<TAPS, T>::init() {

    bool b = false;

    if (filter_type == BPF) {
        b = generate_fir_filter_taps(filter_type, coeffs, TAPS, this->input_rate, start_frequency, this->output_rate);
    } else {
        b = generate_fir_filter_taps(filter_type, coeffs, TAPS, this->input_rate, this->output_rate, 0);
    }

    dsp_fir_decimate_instance.M = this->factor;
    initialized = b;
    clear_state();
    return initialized;
}

template <int TAPS> bool DspFIRDecimatorFloat<TAPS, complex_t_f32>::init() {

    bool b = false;

    if (this->filter_type == BPF) {
        b = generate_fir_filter_taps(this->filter_type, this->coeffs, TAPS, this->input_rate, this->start_frequency, this->output_rate);
    } else {
        b = generate_fir_filter_taps(this->filter_type, this->coeffs, TAPS, this->input_rate, this->output_rate, 0);
    }
    dsp_fir_decimate_instance.M = this->factor;
    this->initialized = b;
    clear_state();
    return this->initialized;
}

template <int TAPS, typename T> bool DspFIRDecimatorFloatBase<TAPS, T>::config(uint32_t input_rate, uint32_t output_rate, uint16_t f, uint32_t start_freq) {

    this->input_rate = input_rate;
    this->output_rate = output_rate;
    this->factor = f;
    if (start_freq) {
        filter_type = BPF;
        start_frequency = start_freq;
    } else {
        filter_type = LPF;
        start_frequency = 0;
    }
    return init();
}

template <int TAPS> void DspFIRDecimatorFloat<TAPS, complex_t_f32>::clear_state() {
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(state));
    memset(dsp_fir_decimate_instance_q.pState, 0, sizeof(state_q));
}

template <int TAPS, typename T> void DspFIRDecimatorFloatBase<TAPS, T>::clear_state() {
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(state));
}

template <int TAPS, typename T> bool DspFIRDecimatorFloatBase<TAPS, T>::get_initialized() const {
    return initialized;
}

template <int TAPS, typename T> void DspFIRDecimatorFloatBase<TAPS, T>::set_factor(uint16_t factor) {
    this->factor = factor;
    dsp_fir_decimate_instance.M = factor;
}
