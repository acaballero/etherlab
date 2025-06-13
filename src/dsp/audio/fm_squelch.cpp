//
// Created by Angel Dust on 13/06/2025.
//

#include "fm_squelch.h"
#include "dsp/decimation/dsp_decimator.h"
#include <stdio.h>

bool FMSquelch::is_noise(buffer_t<float32_t> &audio) {
    if (threshold == 0.0f) {
        return false;
    }

    // Expects interleaved IQ.
    // TODO: Make it work with real signals so one single DAC is feed

    size_t n = audio.count >> 1;
    float32_t high_freq_samples[n];
    buffer_t<float32_t> high_freq_buffer{high_freq_samples, n};
    high_pass_filter.decimate(audio, high_freq_buffer, 0, 2, 1);

    float high_freq_magnitude = 0;

    for (size_t i = 0; i < high_freq_buffer.count; i++) {
        auto sample = high_freq_buffer.p[i];
        float sample_squared = sample * sample;

        if (sample_squared > high_freq_magnitude) {
            high_freq_magnitude = sample_squared;
        }
    }

    if (high_freq_magnitude > threshold) {
        return true;
    } else {
        return false;
    }
}

void FMSquelch::config(const float threshold, uint32_t sample_rate) {
    this->threshold = threshold * threshold; // square the peak magnitude

    high_pass_filter.config(sample_rate, 4000, 1, HPF);
}

bool FMSquelch::enabled() const {
    return threshold > 0.0;
}
