//
// Created by Angel Dust on 21/04/2021.
//

#ifndef TRX_FRONTEND_DSP_FIR_DECIMATOR_FLOAT_H
#define TRX_FRONTEND_DSP_FIR_DECIMATOR_FLOAT_H

#include "dsp/dsp_common.h"
#include "dsp/fir_filter.h"
#include "memory_allocator.h"
#include "stdio.h"
#include "dsp_decimator.h"
#include "dsp/dsp_buffers.h"
#include "dsp/fft/fft_types.h"
#include "status.h"

template <int TAPS = FFT_LPF_FIR_FILTER_NTAPS, typename T = float> class DspFIRDecimatorFloatBase : public DspDecimator<T> {

  public:
    DspFIRDecimatorFloatBase() : DspDecimator<T>(0){};

    virtual ~DspFIRDecimatorFloatBase() {

        CCMMemoryAllocator::free(coeffs);
        CCMMemoryAllocator::free(state);
        CCMMemoryAllocator::free(tmp_buff);
    }

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
    static constexpr int state_size = (TAPS + DSP_BLOCK - 1) * sizeof(float32_t);

    virtual bool init();

    bool initialized = false;

    filter_type type = LPF;

    uint32_t start_frequency;      // Start frequency for the band-pass case
    float32_t *coeffs = nullptr;   //[TAPS];
    float32_t *state = nullptr;    //[TAPS + DSP_BLOCK - 1];
    float32_t *tmp_buff = nullptr; //[DSP_BLOCK];

    arm_fir_decimate_instance_f32 dsp_fir_decimate_instance = {1, TAPS, coeffs, state};
};

template <int TAPS = FFT_LPF_FIR_FILTER_NTAPS> class DspFIRDecimatorFloat : public DspFIRDecimatorFloatBase<TAPS, float32_t> {
  public:
    ~DspFIRDecimatorFloat() override {

        CCMMemoryAllocator::free(state_q);
        CCMMemoryAllocator::free(tmp_buff_q);
    }

    void set_factor(uint16_t factor) override;

    void decimate(buffer_t<float32_t> &src, buffer_t<float32_t> &dst) override;
    void decimate(float32_t *src_i, float32_t *src_q, float32_t *dst_i, float32_t *dst_q, size_t n_samples) override;
    void decimate(buffer_t<float32_t> &src, buffer_t<float32_t> &dst, uint8_t start, uint8_t n_channels, int start_dst = -1);
    void decimate(buffer_t<complex_t_f32> &src, float *dst_i, float *dst_q);
    void decimate(float *src_i, float *src_q, buffer_t<float32_t> &dst, size_t n_samples);

    bool config(uint32_t input_rate, uint32_t output_rate, uint16_t factor, uint32_t start_frequency = 0) override;

  protected:
    using DspFIRDecimatorFloatBase<TAPS, float32_t>::state;
    using DspFIRDecimatorFloatBase<TAPS, float32_t>::state_size;
    using DspFIRDecimatorFloatBase<TAPS, float32_t>::tmp_buff;
    using DspFIRDecimatorFloatBase<TAPS, float32_t>::dsp_fir_decimate_instance;

    bool init() override;
    void clear_state() override;

    float32_t *state_q = nullptr;    //[TAPS + DSP_BLOCK - 1];
    float32_t *tmp_buff_q = nullptr; // DSP_BLOCK

    arm_fir_decimate_instance_f32 dsp_fir_decimate_instance_q = {1, TAPS, this->coeffs, state_q};
};

#endif // TRX_FRONTEND_DSP_FIR_DECIMATOR_FLOAT_H
