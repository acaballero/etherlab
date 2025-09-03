//
// Created by Angel Dust on 13/06/2025.
//

#include "fm_squelch.h"
#include "dsp/decimation/dsp_decimator.h"
#include <stdio.h>
#include "status.h"

bool FMSquelch::is_noise(buffer_t<float32_t> &audio) {
    if (threshold == 0.0f) {
        return false;
    }

    size_t n = audio.count;
    float32_t high_freq_samples[n];
    buffer_t<float32_t> high_freq_buffer{high_freq_samples, n};
    high_pass_filter.decimate(audio, high_freq_buffer, 0, 1, 1);

    float high_freq_magnitude = 0;

    for (size_t i = 0; i < n; i++) {
        auto sample = high_freq_buffer.p[i];
        float sample_squared = sample * sample;
        high_freq_magnitude += sample_squared;
    }
    high_freq_magnitude /= n;
    // LOG("%5.1f,%5.1f,%5.1f\n", high_freq_magnitude_max, high_freq_magnitude_min, high_freq_magnitude);
    // LOG("%5.1f\n", high_freq_magnitude);

    bool is_noise = high_freq_magnitude > threshold;

    audio_history = (audio_history << 1) | (is_noise ? 0 : 1);
    is_noise = audio_history == 0;

    if (is_noise) {
        return true;
    } else {
        return false;
    }
}

void FMSquelch::config(const float mag_threshold, uint32_t sample_rate, uint32_t audio_bandwidth) {

    this->threshold = 5 * mag_threshold * mag_threshold; // square the peak magnitude
    high_pass_filter.config(sample_rate, audio_bandwidth, 1, HPF);
}

bool FMSquelch::enabled() const {
    return threshold > 0.0;
}
