//
// Created by Angel Dust on 13/06/2025.
//

#ifndef __FM_SQUELCH_H__
#define __FM_SQUELCH_H__

#include "dsp/buffer.hpp"
#include "dsp/decimation/dsp_iir_decimator.h"
#include <cstdint>
#include <cstddef>

class FMSquelch {
  public:
    /* Returns true if noise is above threshold. */
    bool is_noise(buffer_t<float32_t> &audio);
    void config(const float new_threshold, uint32_t sample_rate, uint32_t audio_bandwidth = 4000);
    bool enabled() const;

  private:
    float threshold{0.0f};
    DspIIRDecimator<2> high_pass_filter;

    // With only one block of samples some false positives may appear
    // We could measure (relative) variance, but just a simple audio history
    // is usually enough to delay the noise detection a few ms.
    // 64,32..8 bits can be used, depending on the length of the delay line we need
    // NOTE: Even with only 8 bits the squelch is not as fast as I want
    // However, i've tested it only with narrow bandwidth FM, so the deviation is small and the signal is naturally limited in amplitude.
    // With larger deviation, the 2nd order HPF would not have enough attenuation and, for example, a high amplitude tone may fire the threshold.
    uint16_t audio_history{0};
    bool was_noise;
};

#endif /*__FM_SQUELCH_H__*/
