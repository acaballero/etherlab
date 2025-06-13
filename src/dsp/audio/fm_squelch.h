//
// Created by Angel Dust on 13/06/2025.
//

#ifndef __FM_SQUELCH_H__
#define __FM_SQUELCH_H__

#include "dsp/buffer.hpp"
#include "dsp/decimation/dsp_decimators.h"
#include <cstdint>
#include <cstddef>

class FMSquelch {
  public:
    /* Returns true if noise is above threshold. */
    bool is_noise(buffer_t<float32_t> &audio);
    void config(const float new_threshold, uint32_t sample_rate);
    bool enabled() const;

  private:
    float threshold{0.0f};
    DspIIRDecimator<2> high_pass_filter;
};

#endif /*__FM_SQUELCH_H__*/
