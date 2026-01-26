//
// Created by Angel Dust on 25/01/2026.
//

#ifndef TRX_FRONTEND_DSP_FIR_INTERPOLATOR_Q15_H
#define TRX_FRONTEND_DSP_FIR_INTERPOLATOR_Q15_H

#include "dsp/dsp_common.h"
#include "dsp/fir_filter.h"
#include "dsp_interpolator.h"
#include "stdio.h"
#include "dsp/dsp_buffers.h"
#include "memory_allocator.h"

template <int TAPS, typename T> class DspFIRInterpolatorQ15Base : public DspInterpolator<T> {

  public:
    DspFIRInterpolatorQ15Base() : DspInterpolator<T>(0){};

    DspFIRInterpolatorQ15Base(uint32_t input_rate, uint32_t bandwidth, uint16_t factor) : DspInterpolator<T>(input_rate, bandwidth, factor) {
        init();
    };

    virtual bool config(uint32_t input_rate, uint32_t output_rate, uint16_t factor);
    virtual void clear_state();
    bool get_initialized() const;
    void set_factor(uint16_t factor);

  protected:
    virtual bool init();

    bool initialized = false;

    q15_t coeffs[TAPS];
    q15_t state[TAPS + DSP_BLOCK];
    q15_t tmp_buff_in[DSP_BLOCK];
    q15_t tmp_buff_out[DSP_BLOCK];

    arm_fir_interpolate_instance_q15 dsp_fir_interpolate_instance = {(uint8_t)1, TAPS, coeffs, state};
};
template <int TAPS = FFT_LPF_FIR_FILTER_NTAPS, typename T = complex_t> class DspFIRInterpolatorQ15 : public DspFIRInterpolatorQ15Base<TAPS, T> {

  public:
    void interpolate(buffer_t<T> &src, buffer_t<T> &dst) override;
    // Specialization for interpolating interleaved buffers
    void interpolate(buffer_t<T> &src, buffer_t<T> &dst, uint8_t start, uint8_t n_channels);
};

/**
 * Specialization for complex Q15 buffers
 */
template <int TAPS> class DspFIRInterpolatorQ15<TAPS, complex_t> : public DspFIRInterpolatorQ15Base<TAPS, complex_t> {
  public:
    void interpolate(buffer_t<complex_t> &src, buffer_t<complex_t> &dst) override;
    void interpolate(buffer_t<complex_t> &src, adc_type *dst_i, adc_type *dst_q);
    void interpolate(adc_type *src_i, adc_type *src_q, adc_type *dst_i, adc_type *dst_q, size_t n_samples);
    void interpolate(adc_type *src_i, adc_type *src_q, buffer_t<complex_t> &dst, size_t n_samples);
    void clear_state() override;

  protected:
    using DspFIRInterpolatorQ15Base<TAPS, complex_t>::state;
    using DspFIRInterpolatorQ15Base<TAPS, complex_t>::tmp_buff_in;
    using DspFIRInterpolatorQ15Base<TAPS, complex_t>::tmp_buff_out;
    using DspFIRInterpolatorQ15Base<TAPS, complex_t>::dsp_fir_interpolate_instance;

    bool init() override;

    q15_t state_q[TAPS + DSP_BLOCK - 1];
    q15_t tmp_buff_in_q[DSP_BLOCK];
    q15_t tmp_buff_out_q[DSP_BLOCK];
    arm_fir_interpolate_instance_q15 dsp_fir_interpolate_instance_q = {1, TAPS, this->coeffs, state_q};
};

#endif // TRX_FRONTEND_DSP_FIR_INTERPOLATOR_Q15_H
