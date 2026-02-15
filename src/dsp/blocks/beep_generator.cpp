//
// Created by Angel Dust on 09/09/2025.
//
#include "beep_generator.h"
#include "arm_math.h"
#include "dsp/dsp_common.h"
#include "status.h"
#include "utils.hpp"
namespace dsp {
// Predefined pleasant beep configurations
const beep_params_st BeepGenerator::BEEP_CONFIGS[BEEP_TYPE_COUNT] = {
    // BEEP_SINGLE - gentle single beep
    {880, 0, 150, 0, 1, 0.7f, SIGNAL_SHAPE_SIN, 10},

    // BEEP_DOUBLE - two quick beeps
    {880, 0, 120, 150, 2, 0.7f, SIGNAL_SHAPE_SIN, 10},

    // BEEP_TRIPLE - three quick beeps
    {880, 0, 100, 120, 3, 0.7f, SIGNAL_SHAPE_SIN, 8},

    // BEEP_TWO_TONE - alternating frequencies
    {880, 1109, 250, 100, 2, 0.8f, SIGNAL_SHAPE_SIN, 15},

    // BEEP_ASCENDING - rising tone pattern
    {659, 880, 200, 50, 3, 0.8f, SIGNAL_SHAPE_SIN, 15},

    // BEEP_SUCCESS - pleasant success sound
    {440, 870, 70, 90, 1, 0.8f, SIGNAL_SHAPE_SIN, 25},

    // BEEP_WARNING - attention-getting but not harsh
    {800, 1000, 300, 50, 3, 0.9f, SIGNAL_SHAPE_SIN, 20},

    // BEEP_ERROR - lower frequency, more urgent
    {400, 600, 400, 200, 2, 0.9f, SIGNAL_SHAPE_SIN, 30}};

// Implementation
BeepGenerator::BeepGenerator() : phase{false, false, 0, 0, false}, timing{0, 0, 0} {
}

void BeepGenerator::set_sample_rate(uint32_t sample_rate) {
    signal_gen.set_sample_rate(sample_rate);
}

beep_params_st BeepGenerator::get_beep_config(BEEP_TYPE type) {
    return (type >= 0 && type < BEEP_TYPE_COUNT) ? BEEP_CONFIGS[type] : BEEP_CONFIGS[BEEP_SINGLE];
}

void BeepGenerator::init(BEEP_TYPE type) {
    init(get_beep_config(type));
}

void BeepGenerator::init(const beep_params_st &p) {
    params = p;

    // Calculate timing
    uint32_t sr = signal_gen.get_sample_rate();
    timing.beep_samples = (params.duration_ms * sr) / 1000;
    timing.pause_samples = (params.pause_ms * sr) / 1000;
    timing.fade_samples = (params.fade_ms * sr) / 1000;

    // Reset state
    phase = {true, true, 0, 0, false};

    configure_signal();
}

void BeepGenerator::configure_signal() {
    uint32_t freq = phase.use_second_freq ? params.frequency2 : params.frequency1;
    signal_gen.set_frequency(freq);
    signal_gen.set_shape(params.shape);
    signal_gen.init();
}

float BeepGenerator::current_envelope() {

    float gain = 1.0f;

    if (!phase.active || !phase.in_beep) {
        gain = 0.0f;
    }

    // Fade in
    else if (phase.sample_count < timing.fade_samples) {
        gain = (float)phase.sample_count / timing.fade_samples;
    }
    // Fade out
    else if (phase.sample_count >= timing.beep_samples - timing.fade_samples) {
        uint32_t fade_pos = phase.sample_count - (timing.beep_samples - timing.fade_samples);
        gain = 1.0f - ((float)fade_pos / timing.fade_samples);
    }

    return gain * params.gain;
}

void BeepGenerator::phase_advance() {

    if (!phase.active) {
        return;
    }

    phase.sample_count++;

    if (phase.in_beep) {
        // End of beep?
        if (phase.sample_count >= timing.beep_samples) {
            phase.sample_count = 0;

            // Switch to second frequency for two-tone beeps
            if (params.frequency2 > 0 && !phase.use_second_freq) {
                phase.use_second_freq = true;
                configure_signal();

            } else {

                phase.in_beep = false;
            }
        }
    } else {
        // End of pause?
        if (phase.sample_count >= timing.pause_samples) {
            phase.sample_count = 0;
            phase.current_rep++;

            if (phase.current_rep < params.repetitions) {
                phase.in_beep = true;
                // Reset to first frequency
                if (phase.use_second_freq) {
                    phase.use_second_freq = false;
                    configure_signal();
                }
            } else {
                phase.active = false; // Sequence complete
            }
        }
    }
}

void BeepGenerator::get_sample(adc_type &sample) {
    if (!phase.active) {
        return;
    }

    if (phase.in_beep) {
        signal_gen.get_sample(sample);
        sample = (adc_type)(sample * current_envelope());
    } else {
        sample = 0;
    }

    phase_advance();
}

void BeepGenerator::get_complex_sample(complex_t &sample) {
    if (!phase.active) {
        sample.r = sample.i = 0;
        return;
    }

    if (phase.in_beep) {
        signal_gen.get_complex_sample(sample);
        float envelope = current_envelope();
        sample.r = (adc_type)(sample.r * envelope);
        sample.i = (adc_type)(sample.i * envelope);
    } else {
        sample.r = sample.i = 0;
    }

    phase_advance();
}

void BeepGenerator::get_block(buffer_t<complex_t> &buffer) {
    complex_t sample;
    for (size_t i = 0; i < buffer.count; i++) {
        get_complex_sample(sample);
        buffer.p[i].i = sample.i;
        buffer.p[i].r = sample.r;
    }
}

void BeepGenerator::stop() {
    phase.active = false;
}

void BeepGenerator::restart() {
    init(params);
}
} // namespace dsp
