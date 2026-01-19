//
// Created by Angel Dust on 29/04/2025.
//

#ifndef TRX_FRONTEND_DSP_FIR_DECIMATOR_FLOAT_COMPLEX_H
#define TRX_FRONTEND_DSP_FIR_DECIMATOR_FLOAT_COMPLEX_H

#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "dsp/dsp_common.h"
#include "dsp/fir_filter.h"
#include "stdio.h"
#include "dsp_decimator.h"
#include "dsp/dsp_buffers.h"
#include "dsp/fft/fft_types.h"

template <int TAPS = FFT_LPF_FIR_FILTER_NTAPS> class DspFIRDecimatorFloatComplex : public DspDecimator<float32_t> {

  public:
    DspFIRDecimatorFloatComplex() : DspDecimator<float32_t>(0){};

    DspFIRDecimatorFloatComplex(uint32_t input_rate, uint32_t start_freq, uint32_t end_freq, uint16_t factor)
        : DspDecimator<float32_t>(input_rate, end_freq, factor), start_frequency{start_freq} {
        this->init();
    };

    virtual ~DspFIRDecimatorFloatComplex() {

        CCMMemoryAllocator::free(coeffs_i);

        CCMMemoryAllocator::free(coeffs_q);
        CCMMemoryAllocator::free(state_xi_hi);
        CCMMemoryAllocator::free(state_xq_hq);
        CCMMemoryAllocator::free(state_xi_hq);
        CCMMemoryAllocator::free(state_xq_hi);
        CCMMemoryAllocator::free(tmp_buff_i);
        CCMMemoryAllocator::free(tmp_buff_q);
    }

    void decimate(buffer_t<float32_t> &src, buffer_t<float32_t> &dst) override;
    void decimate(float *src_i, float *src_q, float *dst_i, float *dst_q, size_t n_samples) override;
    void decimate(float32_t *src, float32_t *dst, size_t n_samples);
    bool config(uint32_t input_rate, uint32_t output_rate, uint16_t factor, uint32_t start_frequency = 0) override;
    virtual void clear_state();
    bool get_initialized() const;

    void set_factor(uint16_t factor) override;

  protected:
    static constexpr int state_size = (TAPS + DSP_BLOCK - 1) * sizeof(float32_t);

    virtual bool init();
    bool initialized = false;

    uint32_t start_frequency; // Start frequency for the band-pass case

    float *coeffs_i{nullptr}; //[TAPS];
    float *coeffs_q{nullptr}; //[TAPS];

    // 4 real filters are required
    // We break a complex FIR into 4 real FIR paths:
    // Let input be complex of the form x[n] = x_r[n] + j·x_i[n]
    //     and complex filter taps h[n] = h_r[n] + j·h_i[n].
    // Then the output is:
    // y_r[n] = FIR(x_r, h_r) - FIR(x_i, h_i)
    // y_i[n] = FIR(x_r, h_i) + FIR(x_i, h_r)

    float *state_xi_hi; //[TAPS + DSP_BLOCK - 1];
    float *state_xq_hq; //[TAPS + DSP_BLOCK - 1];
    float *state_xi_hq; //[TAPS + DSP_BLOCK - 1];
    float *state_xq_hi; //[TAPS + DSP_BLOCK - 1];

    arm_fir_decimate_instance_f32 fir_xi_hi;
    arm_fir_decimate_instance_f32 fir_xq_hq;
    arm_fir_decimate_instance_f32 fir_xi_hq;
    arm_fir_decimate_instance_f32 fir_xq_hi;

    float *tmp_buff_i; //[DSP_BLOCK];
    float *tmp_buff_q; //[DSP_BLOCK];
};

#endif // TRX_FRONTEND_DSP_FIR_DECIMATOR_FLOAT_COMPLEX_H
