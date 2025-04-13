//
// Created by Angel Dust on 11/04/2025.
//
#ifndef __DSP_HILBERT_H__
#define __DSP_HILBERT_H__

#include "dsp/dsp_sos.hpp"

namespace dsp {

class HilbertTransform {
  public:
    HilbertTransform();
    void execute(float in, float &out_i, float &out_q);

  private:
    uint8_t n = 0;
    SOSFilter<5> sos_input = {};
    SOSFilter<5> sos_i = {};
    SOSFilter<5> sos_q = {};
};

class Real_to_Complex {
  public:
    Real_to_Complex();
    void execute(float in, float &out_mag_sq_lpf);

  private:
    uint8_t n = 0;
    SOSFilter<5> sos_input = {};
    SOSFilter<5> sos_i = {};
    SOSFilter<5> sos_q = {};
    SOSFilter<5> sos_mag_sq = {};
};

} /* namespace dsp */

#endif /*__DSP_HILBERT_H__*/
