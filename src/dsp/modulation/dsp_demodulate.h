//
// Created by Angel Dust on 11/04/2025.
//
#ifndef __DSP_DEMODULATE_H__
#define __DSP_DEMODULATE_H__
#include "dsp/dsp_common.h"
#include "dsp/blocks/output.h"
#include "dsp_hilbert.hpp"

namespace dsp {

class demodulator {
  public:
    // Interleaved interfaces
    virtual void work(buffer_t<complex_t_f32> &src, float32_t *dst) = 0;
    virtual void work(buffer_t<complex_t> &src, adc_type *dst) = 0;

    // Unzipped input, interleaved output (for compatibility)
    virtual void work(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) = 0;

    virtual void work_real(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) = 0;
};

class am_demodulator : public demodulator {
  public:
    void work(buffer_t<complex_t_f32> &src, float32_t *dst) override;
    void work(buffer_t<complex_t> &src, adc_type *dst) override;
    void work(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) override;
    void work_real(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) override;
};

class ssb_demodulator : public demodulator {
  public:
    void work(buffer_t<complex_t_f32> &src, float32_t *dst) override;
    void work(buffer_t<complex_t> &src, adc_type *dst) override;
    void work(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) override;
    void work_real(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) override;
};

class ssb_fm_demodulator : public demodulator {
  public:
    void work(buffer_t<complex_t_f32> &src, float32_t *dst) override;
    void work(buffer_t<complex_t> &src, adc_type *dst) override;
    void work(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) override;
    void work_real(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) override;

  private:
    dsp::Real_to_Complex real_to_complex{};
};

class fm_demodulator : public demodulator {
  public:
    void work(buffer_t<complex_t_f32> &src, float32_t *dst) override;
    void work(buffer_t<complex_t> &src, adc_type *dst) override;
    void work(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) override;
    void work_real(const float32_t *src_i, const float32_t *src_q, float32_t *dst_p, size_t count) override;
    void configure(const float sampling_rate, const float deviation_hz);

  private:
    uint32_t z_{0};                   //  Stores a complex packed IQ sample between iterations to help unrolling the loop
    float32_t prev_i_f32, prev_q_f32; // Same for the unzipped version
    complex_t_f32 zcf32_{0, 0};       // Same, but for float version
    float kf{0};
};

} /* namespace dsp */

#endif /*__DSP_DEMODULATE_H__*/
