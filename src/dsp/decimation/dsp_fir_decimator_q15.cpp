//
// Created by Angel Dust on 21/04/2021.
//

#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"
#include "dsp/firFilter.h"
#include "dsp_fir_decimator_q15.h"
#include "dsp/window.h"
#include <cstddef>

template class DspFIRDecimatorQ15Base<FIR_DECIMATOR_1ST_HALFBAND_TAPS, complex_t>;
template class DspFIRDecimatorQ15Base<FIR_DECIMATOR_SIGNAL_TAPS, complex_t>;
template class DspFIRDecimatorQ15<FIR_DECIMATOR_1ST_HALFBAND_TAPS, complex_t>;
template class DspFIRDecimatorQ15<FIR_DECIMATOR_SIGNAL_TAPS, complex_t>;

template <int TAPS, typename T> void DspFIRDecimatorQ15<TAPS, T>::decimate(buffer_t<T> &src, buffer_t<T> &dst) {
    this->decimate(src, dst, 0, 2);
}

/*
 * Decimate a DSP_BLOCK size I/Q sample buffer (I/Q are interleaved)
 * This function is intended to be called either for the I or Q half of the buffer
 * @param Start: 0: Process I samples; 1: Process Q samples
 */

template <int TAPS, typename T> void DspFIRDecimatorQ15<TAPS, T>::decimate(buffer_t<T> &src, buffer_t<T> &dst, uint8_t start, uint8_t n_channels) {

    // TODO: Consider skipping the first processed blocks to account for the delay group of the filter
    uint16_t n_samples = src.count / n_channels;
    uint16_t decimated_block_size = src.count / this->factor;

    // Extract the signal from the interleaved IQ buffer

    for (uint16_t i = start, j = 0; j < n_samples; i += n_channels, j++) {
        this->tmp_buff_in[j] = src.p[i];
    }

    arm_fir_decimate_q15(&this->dsp_fir_decimate_instance, this->tmp_buff_in, this->tmp_buff_out, n_samples);

    // Write to the final adc_buffer in interleaved IQ format
    for (uint16_t i = start, j = 0; j < decimated_block_size; i += 2, j++) {
        dst.p[i] = this->tmp_buff_out[j];
    }
}

template <int TAPS> void DspFIRDecimatorQ15<TAPS, complex_t>::decimate(buffer_t<complex_t> &src, adc_type *dst_i, adc_type *dst_q) {

    uint16_t n_samples = src.count;

    dsp::unzip_c16((const adc_type *)src.p, tmp_buff_in, tmp_buff_in_q, n_samples);

    arm_fir_decimate_q15(&dsp_fir_decimate_instance, tmp_buff_in, dst_i, n_samples);
    arm_fir_decimate_q15(&dsp_fir_decimate_instance_q, tmp_buff_in_q, dst_q, n_samples);
}

template <int TAPS> void DspFIRDecimatorQ15<TAPS, complex_t>::decimate(buffer_t<complex_t> &src, buffer_t<complex_t> &dst) {

    uint16_t n_samples = src.count;
    uint16_t decimated_block_size = n_samples / this->factor; // DMA buffer size (DSP_BLOCK) / decimation factor

    dsp::unzip_c16((const adc_type *)src.p, tmp_buff_in, tmp_buff_in_q, n_samples);

    arm_fir_decimate_q15(&dsp_fir_decimate_instance, tmp_buff_in, tmp_buff_out, n_samples);
    arm_fir_decimate_q15(&dsp_fir_decimate_instance_q, tmp_buff_in_q, tmp_buff_out_q, n_samples);

    dsp::zip_c16(tmp_buff_out, tmp_buff_out_q, (adc_type *)dst.p, decimated_block_size);
}

template <int TAPS> void DspFIRDecimatorQ15<TAPS, complex_t>::decimate(adc_type *src_i, adc_type *src_q, adc_type *dst_i, adc_type *dst_q, size_t n_samples) {
    arm_fir_decimate_q15(&dsp_fir_decimate_instance, src_i, dst_i, n_samples);
    arm_fir_decimate_q15(&dsp_fir_decimate_instance_q, src_q, dst_q, n_samples);
}

template <int TAPS> void DspFIRDecimatorQ15<TAPS, complex_t>::decimate(adc_type *src_i, adc_type *src_q, buffer_t<complex_t> &dst, size_t n_samples) {

    uint16_t decimated_block_size = n_samples / this->factor; // DMA buffer size (DSP_BLOCK) / decimation factor

    arm_fir_decimate_q15(&dsp_fir_decimate_instance, src_i, tmp_buff_out, n_samples);
    arm_fir_decimate_q15(&dsp_fir_decimate_instance_q, src_q, tmp_buff_out_q, n_samples);

    dsp::zip_c16(tmp_buff_out, tmp_buff_out_q, (adc_type *)dst.p, decimated_block_size);
}

template <int TAPS, typename T> bool DspFIRDecimatorQ15Base<TAPS, T>::init() {

    bool ret = generate_fir_filter_taps_q15(LPF, coeffs, TAPS, this->input_rate, this->output_rate, 0);

    dsp_fir_decimate_instance.M = this->factor;
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(state));

    initialized = ret;
    return ret;
}

template <int TAPS> bool DspFIRDecimatorQ15<TAPS, complex_t>::init() {

    bool ret = DspFIRDecimatorQ15Base<TAPS, complex_t>::init();

    dsp_fir_decimate_instance_q.M = this->factor;
    memset(dsp_fir_decimate_instance_q.pState, 0, sizeof(state_q));

    return ret;
}

template <int TAPS, typename T> bool DspFIRDecimatorQ15Base<TAPS, T>::config(uint32_t input_rate, uint32_t output_rate, uint16_t factor) {

    this->input_rate = input_rate;
    this->output_rate = output_rate;
    this->factor = factor;
    return this->init();
}

template <int TAPS> void DspFIRDecimatorQ15<TAPS, complex_t>::clear_state() {
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(state));
    memset(dsp_fir_decimate_instance_q.pState, 0, sizeof(state_q));
}

template <int TAPS, typename T> void DspFIRDecimatorQ15Base<TAPS, T>::clear_state() {
    memset(dsp_fir_decimate_instance.pState, 0, sizeof(state));
}

template <int TAPS, typename T> bool DspFIRDecimatorQ15Base<TAPS, T>::get_initialized() const {
    return initialized;
}
template <int TAPS, typename T> void DspFIRDecimatorQ15Base<TAPS, T>::set_factor(uint16_t factor) {
    this->factor = factor;
    init();
}
