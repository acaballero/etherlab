//
// Created by Angel Dust on 21/04/2021.
//

#ifndef TRX_FRONTEND_DSP_FIR_DECIMATOR_FLOAT_H
#define TRX_FRONTEND_DSP_FIR_DECIMATOR_FLOAT_H

#include "dsp/dsp_common.h"
#include "dsp/fir_filter.h"
#include "stdio.h"
#include "dsp_decimator.h"
#include "dsp/dsp_buffers.h"
#include "dsp/fft/fft_types.h"

class IDspDecimatorFloat {
  public:
    virtual void decimate(float *src_i, float *src_q, float *dst_i, float *dst_q, size_t n_samples) = 0;
    virtual bool config(uint32_t input_rate, uint32_t bandwidth, uint16_t factor, uint32_t start_frequency = 0) = 0;

    virtual ~IDspDecimatorFloat() = default;
};

template <int TAPS = FFT_LPF_FIR_FILTER_NTAPS, typename T = float> class DspFIRDecimatorFloatBase : public DspDecimator<T> {

  public:
    DspFIRDecimatorFloatBase() : DspDecimator<T>(0){};

    DspFIRDecimatorFloatBase(uint32_t input_rate, uint32_t output_rate, uint16_t factor) : DspDecimator<T>(input_rate, output_rate, factor) {
        this->init();
    };
    DspFIRDecimatorFloatBase(uint32_t input_rate, uint32_t start_freq, uint32_t end_freq, uint16_t factor)
        : DspDecimator<T>(input_rate, end_freq, factor), type{BPF}, start_frequency{start_freq} {
        this->init();
    };

    bool get_initialized() const;

    void set_factor(uint16_t factor) override;

    virtual void clear_state();

  protected:
    virtual bool init();

    bool initialized = false;

    filter_type type = LPF;

    uint32_t start_frequency; // Start frequency for the band-pass case
    float coeffs[TAPS];
    float state[TAPS + DSP_BLOCK - 1];
    float tmp_buff_in[DSP_BLOCK];
    float tmp_buff_out[DSP_BLOCK];

    arm_fir_decimate_instance_f32 dsp_fir_decimate_instance = {1, TAPS, coeffs, state};
};

template <int TAPS = FFT_LPF_FIR_FILTER_NTAPS, typename T = float> class DspFIRDecimatorFloat : public DspFIRDecimatorFloatBase<TAPS, T> {

  public:
    void decimate(buffer_t<T> &src, buffer_t<T> &dst) override;
    void decimate(buffer_t<T> &src, buffer_t<T> &dst, uint8_t start, uint8_t n_channels);

    bool config(uint32_t input_rate, uint32_t output_rate, uint16_t factor, uint32_t start_frequency = 0);
};

/**
 * Specialization for complex float_32 buffers
 */
template <int TAPS> class DspFIRDecimatorFloat<TAPS, complex_t_f32> : public DspFIRDecimatorFloatBase<TAPS, complex_t_f32>, public IDspDecimatorFloat {
  public:
    void set_factor(uint16_t factor) override;
    void decimate(buffer_t<complex_t_f32> &src, buffer_t<complex_t_f32> &dst) override;
    void decimate(float *src_i, float *src_q, float *dst_i, float *dst_q, size_t n_samples) override;

    void decimate(buffer_t<complex_t_f32> &src, float *dst_i, float *dst_q);
    void decimate(float *src_i, float *src_q, buffer_t<complex_t_f32> &dst, size_t n_samples);

    bool config(uint32_t input_rate, uint32_t output_rate, uint16_t factor, uint32_t start_frequency = 0) override;

  protected:
    using DspFIRDecimatorFloatBase<TAPS, complex_t_f32>::state;
    using DspFIRDecimatorFloatBase<TAPS, complex_t_f32>::tmp_buff_in;
    using DspFIRDecimatorFloatBase<TAPS, complex_t_f32>::tmp_buff_out;
    using DspFIRDecimatorFloatBase<TAPS, complex_t_f32>::dsp_fir_decimate_instance;

    bool init() override;
    void clear_state() override;

    float state_q[TAPS + DSP_BLOCK - 1];
    float tmp_buff_in_q[DSP_BLOCK];
    float tmp_buff_out_q[DSP_BLOCK];
    arm_fir_decimate_instance_f32 dsp_fir_decimate_instance_q = {1, TAPS, this->coeffs, state_q};
};

#endif // TRX_FRONTEND_DSP_FIR_DECIMATOR_FLOAT_H
