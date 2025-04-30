//
// Created by Angel Dust on 27/04/2025.
//

#ifndef __AUDIO_COMPRESSOR_H__
#define __AUDIO_COMPRESSOR_H__

#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"

#include <cmath>

/* Code based on article in Journal of the Audio Engineering Society
 * Vol. 60, No. 6, 2012 June, by Dimitrios Giannoulis, Michael Massberg,
 * Joshua D. Reiss "Digital Dynamic Range Compressor Design – A Tutorial
 * and Analysis"
 */

class GainComputer {
  public:
    constexpr GainComputer(float ratio, float threshold) : ratio{ratio}, slope{1.0f / ratio - 1.0f}, threshold_db{threshold} {
    }

    float operator()(const float x) const;

  private:
    const float ratio;
    const float slope;
    const float threshold_db;

    static constexpr float knee_width_db = 0.0f;

    static constexpr float db_floor = -120.0f;
    static constexpr float lin_floor = 0.000001;      // std::pow(10.0f, db_floor / 20.0f);
    static constexpr float log2_db_k = 6.02059991328; // 20.0f * std::log10(2.0f);
};

class PeakDetectorBranchingSmooth {
  public:
    constexpr PeakDetectorBranchingSmooth(float att_a, float rel_a) : att_a{att_a}, rel_a{rel_a} {
    }

    float operator()(const float db) {
        const auto a = (db > state) ? att_a : rel_a;
        state = db + a * (state - db);
        return state;
    }

  private:
    float state{0.0f};
    const float att_a;
    const float rel_a;
};

class FeedForwardCompressor {
  public:
    void work(const buffer_t<float32_t> &buffer);

  private:
    static constexpr float fs = 12000.0f;
    static constexpr float ratio = 10.0f;
    static constexpr float threshold = -30.0f;
    static constexpr float makeup_gain = 0.04466835921; // std::pow(10.0f, (threshold - (threshold / ratio)) / -20.0f);

    GainComputer gain_computer{ratio, threshold};
    PeakDetectorBranchingSmooth peak_detector{tau_alpha(0.010f, fs), tau_alpha(0.300f, fs)};

    float work(const float x);

    static constexpr float tau_alpha(const float tau, const float fs) {
        return std::exp(-1.0f / (tau * fs));
    }
};

#endif /*__AUDIO_COMPRESSOR_H__*/
