//
// Created by Angel Dust on 21/04/2021.
//

#ifndef TRX_FRONTEND_DSP_FIR_DECIMATOR_Q15_H
#define TRX_FRONTEND_DSP_FIR_DECIMATOR_Q15_H

#include "arm_math.h"
#include "dsp/dsp_common.h"
#include "stdio.h"
#include "dsp_decimator.h"
#include "dsp/dsp_buffers.h"
#include "dsp/fft/fft_types.h"

template <int TAPS = FFT_LPF_FIR_FILTER_NTAPS, typename T = complex_t> class DspFIRDecimatorQ15Base : public DspDecimator<T> {

  public:
    DspFIRDecimatorQ15Base() : DspDecimator<T>(0){};

    DspFIRDecimatorQ15Base(uint32_t input_rate, uint32_t output_rate, uint16_t factor) : DspDecimator<T>(input_rate, output_rate, factor) {
        this->init();
    };

    bool config(uint32_t input_rate, uint32_t output_rate, uint16_t factor);
    void clear_state();
    bool get_initialized() const;
    void set_factor(uint16_t factor);

  protected:
    bool init();

    bool initialized = false;
    q15_t coeffs[TAPS];
    q15_t state[TAPS + DSP_BLOCK - 1];
    q15_t tmp_buff_in[DSP_BLOCK];
    q15_t tmp_buff_out[DSP_BLOCK];
    arm_fir_decimate_instance_q15 dsp_fir_decimate_instance = {1, TAPS, coeffs, state};
};

template <int TAPS = FFT_LPF_FIR_FILTER_NTAPS, typename T = complex_t> class DspFIRDecimatorQ15 : public DspFIRDecimatorQ15Base<TAPS, T> {

  public:
    void decimate(buffer_t<T> &src, buffer_t<T> &dst) override;
    // Specialization for decimating interleaved buffers
    void decimate(buffer_t<T> &src, buffer_t<T> &dst, uint8_t start, uint8_t n_channels);
};

/**
 * Specialization for complex Q15 buffers
 */
template <int TAPS> class DspFIRDecimatorQ15<TAPS, complex_t> : public DspFIRDecimatorQ15Base<TAPS, complex_t> {
  public:
    void decimate(buffer_t<complex_t> &src, buffer_t<complex_t> &dst) override;
    void clear_state();

  protected:
    using DspFIRDecimatorQ15Base<TAPS, complex_t>::state;
    using DspFIRDecimatorQ15Base<TAPS, complex_t>::tmp_buff_in;
    using DspFIRDecimatorQ15Base<TAPS, complex_t>::tmp_buff_out;
    using DspFIRDecimatorQ15Base<TAPS, complex_t>::dsp_fir_decimate_instance;

    bool init();

    q15_t state_q[TAPS + DSP_BLOCK - 1];
    q15_t tmp_buff_in_q[DSP_BLOCK];
    q15_t tmp_buff_out_q[DSP_BLOCK];
    arm_fir_decimate_instance_q15 dsp_fir_decimate_instance_q = {1, TAPS, this->coeffs, state_q};
};

#endif // TRX_FRONTEND_DSP_FIR_DECIMATOR_Q15_H
