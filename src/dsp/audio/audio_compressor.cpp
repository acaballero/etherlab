//
// Created by Angel Dust on 27/04/2025.
//

#include "audio_compressor.hpp"
#include "arm_math.h"
#include "dsp/buffer.hpp"
#include "utils.hpp"

float GainComputer::operator()(const float x) const {
    const auto abs_x = std::abs(x);
    const auto db = (abs_x < lin_floor) ? db_floor : log2_db_k * fasterlog(abs_x);
    const auto overshoot_db = db - threshold_db;
    if (knee_width_db > 0.0f) {
        const auto w2 = knee_width_db / 2.0f;
        const auto a = w2 / (knee_width_db * knee_width_db);
        const auto in_transition = (overshoot_db > -w2) && (overshoot_db < w2);
        const auto rectified_overshoot = in_transition ? (a * std::pow(overshoot_db + w2, 2.0f)) : std::max(overshoot_db, 0.0f);
        return rectified_overshoot * slope;
    } else {
        const auto rectified_overshoot = std::max(overshoot_db, 0.0f);
        return rectified_overshoot * slope;
    }
}

void FeedForwardCompressor::work(const buffer_t<float32_t> &buffer) {

    // Expects interleaved i,q samples
    // TODO: Currently, on receive, the DAC processor expects complex signals (TX mode heritage) Make it work with real signals for RX.
    for (size_t i = 0; i < buffer.count * 2; i += 2) {
        buffer.p[i] = work(buffer.p[i]) * makeup_gain;
    }
}

float FeedForwardCompressor::work(const float x) {
    const auto gain_db = gain_computer(x);
    const auto peak_db = -peak_detector(-gain_db);
    const auto gain = fastpow2(peak_db * (3.321928094887362f / 20.0f));
    return x * gain;
}
