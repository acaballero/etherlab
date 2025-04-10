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
    virtual void work(buffer_t<complex_t> &src, buffer_t<adc_type> &dsc);
};

class am_demodulator : demodulator {
  public:
    void work(buffer_t<complex_t> &src, buffer_t<adc_type> &dsc) override;

  private:
    static constexpr float k = 1.0f / 32768.0f;
};

class ssb_demodulator : demodulator {
  public:
    void work(buffer_t<complex_t> &src, buffer_t<adc_type> &dsc) override;

  private:
    static constexpr float k = 1.0f / 32768.0f;
};

class ssb_fm_demodulator : demodulator {
  public:
    void work(buffer_t<complex_t> &src, buffer_t<adc_type> &dsc) override;

  private:
    static constexpr float k = 1.0f / 32768.0f;
    dsp::Real_to_Complex real_to_complex{}; // It is a member variable of SSB_FM.
};

class fm_demodulator : demodulator {
  public:
    void work(buffer_t<complex_t> &src, buffer_t<adc_type> &dsc) override;
    void configure(const float sampling_rate, const float deviation_hz);

  private:
    complex_t z_{0};
    float kf{0};
    float ks16{0};
};

} /* namespace dsp */

#endif /*__DSP_DEMODULATE_H__*/
