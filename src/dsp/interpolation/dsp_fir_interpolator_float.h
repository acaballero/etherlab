//
// Created by Angel Dust on 25/01/2026.
//

#ifndef TRX_FRONTEND_DSP_FIR_INTERPOLATOR_FLOAT_H
#define TRX_FRONTEND_DSP_FIR_INTERPOLATOR_FLOAT_H

#include "dsp/dsp_common.h"
#include "dsp/fir_filter.h"
#include "memory_allocator.h"
#include "stdio.h"
#include "dsp_interpolator.h"
#include "dsp/dsp_buffers.h"
#include "dsp/fft/fft_types.h"
#include "status.h"

template <int TAPS = FFT_LPF_FIR_FILTER_NTAPS, typename T = float> class DspFIRInterpolatorFloatBase : public DspInterpolator<T> {

  public:
    DspFIRInterpolatorFloatBase() : DspInterpolator<T>(0){};

    virtual ~DspFIRInterpolatorFloatBase() {
        CCMMemoryAllocator::free(coeffs);
        CCMMemoryAllocator::free(state);
    }

    DspFIRInterpolatorFloatBase(uint32_t input_rate, uint32_t output_rate, uint16_t factor) : DspInterpolator<T>(input_rate, output_rate, factor) {
        this->init();
    };

    DspFIRInterpolatorFloatBase(uint32_t input_rate, uint32_t start_freq, uint32_t end_freq, uint16_t factor)
        : DspInterpolator<T>(input_rate, end_freq, factor), type{BPF}, start_frequency{start_freq} {
        this->init();
    };

    bool get_initialized() const;

    void set_factor(uint16_t factor) override;

    void reset() override;

  protected:
    static constexpr int state_size = (TAPS + DSP_BLOCK - 1) * sizeof(float32_t);

    virtual bool init();

    bool initialized = false;

    filter_type type = LPF;

    uint32_t start_frequency; // Start frequency for the band-pass case
    float32_t *coeffs = nullptr;
    float32_t *state = nullptr;
    float tmp_buff_in[DSP_BLOCK];
    float tmp_buff_out[DSP_BLOCK]; // Larger for interpolated output

    arm_fir_interpolate_instance_f32 dsp_fir_interpolate_instance = {(uint8_t)1, TAPS, coeffs, state};
};

template <int TAPS = FFT_LPF_FIR_FILTER_NTAPS> class DspFIRInterpolatorFloat : public DspFIRInterpolatorFloatBase<TAPS, float32_t> {
  public:
    ~DspFIRInterpolatorFloat() override {
        CCMMemoryAllocator::free(state_q);
    }

    void set_factor(uint16_t factor) override;

    void interpolate(buffer_t<float32_t> &src, buffer_t<float32_t> &dst) override;
    void interpolate(float32_t *src_i, float32_t *src_q, float32_t *dst_i, float32_t *dst_q, size_t n_samples) override;
    void interpolate(buffer_t<float32_t> &src, buffer_t<float32_t> &dst, uint8_t start, uint8_t n_channels, int start_dst = -1);
    void interpolate(buffer_t<complex_t_f32> &src, float32_t *dst_i, float32_t *dst_q);
    void interpolate(float32_t *src_i, float32_t *src_q, buffer_t<float32_t> &dst, size_t n_samples);

    bool config(uint32_t input_rate, uint32_t bandwidth, uint16_t factor, uint32_t start_frequency = 0) override;

    void reset() override;

  protected:
    using DspFIRInterpolatorFloatBase<TAPS, float32_t>::state;
    using DspFIRInterpolatorFloatBase<TAPS, float32_t>::state_size;
    using DspFIRInterpolatorFloatBase<TAPS, float32_t>::tmp_buff_in;
    using DspFIRInterpolatorFloatBase<TAPS, float32_t>::tmp_buff_out;
    using DspFIRInterpolatorFloatBase<TAPS, float32_t>::dsp_fir_interpolate_instance;
    bool init() override;

    float tmp_buff_in_q[DSP_BLOCK];
    float tmp_buff_out_q[DSP_BLOCK];

    float32_t *state_q = nullptr;

    arm_fir_interpolate_instance_f32 dsp_fir_interpolate_instance_q = {(uint8_t)1, TAPS, this->coeffs, state_q};
};

#endif // TRX_FRONTEND_DSP_FIR_INTERPOLATOR_FLOAT_H
