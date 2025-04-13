//
// Created by Angel Dust on 11/04/2025.
//
#ifndef __DSP_DEMODULATE_H__
#define __DSP_DEMODULATE_H__

#include "dsp/dsp_common.h"
#include "dsp/blocks/output.h"
#include "dsp_hilbert.hpp"
#include <sys/_stdint.h>

namespace dsp {

class demodulator {
  public:
    virtual void work(buffer_t<complex_t> &src, buffer_t<adc_type> &dsc) = 0;
};

class am_demodulator : public demodulator {
  public:
    void work(buffer_t<complex_t> &src, buffer_t<adc_type> &dsc) override;
};

class ssb_demodulator : public demodulator {
  public:
    void work(buffer_t<complex_t> &src, buffer_t<adc_type> &dsc) override;
};

class ssb_fm_demodulator : public demodulator {
  public:
    void work(buffer_t<complex_t> &src, buffer_t<adc_type> &dsc) override;

  private:
    dsp::Real_to_Complex real_to_complex{};
};

class fm_demodulator : public demodulator {
  public:
    void work(buffer_t<complex_t> &src, buffer_t<adc_type> &dsc) override;
    void configure(const float sampling_rate, const float deviation_hz);

  private:
    uint32_t z_{0};
    float kf{0};
    float ks16{0};
};

} /* namespace dsp */

#endif /*__DSP_DEMODULATE_H__*/
