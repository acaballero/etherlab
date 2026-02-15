//
// Created by Angel Dust on 02/07/2024.
//

#include "noise_generator.h"
#include "config.h"

namespace dsp {
adc_type NoiseGenerator::get_sample() {

    int8_t sample;

    sample = (noise_seed & 0xFF000000) >> 24;
    feedback = ((noise_seed >> 31) ^ (noise_seed >> 29) ^ (noise_seed >> 15) ^ (noise_seed >> 11)) & 1;
    noise_seed = (noise_seed << 1) | feedback;
    if (!noise_seed) {
        noise_seed = 0x1337; // TODO: This is ugly
    }

    return sample;
}

void NoiseGenerator::get_sample(adc_type &sample) {
    sample = get_sample();
}

void NoiseGenerator::get_complex_sample(complex_t &sample) {

    //        if (!sample_count && auto_off) {
    //            txprogress_message.done = true;
    //            shared_memory.application_queue.push(txprogress_message);
    //        } else
    //            sample_count--;

    sample.r = get_sample();
    sample.i = get_sample();
}

void NoiseGenerator::get_block(buffer_t<complex_t> &buffer) {
    complex_t sample;
    for (size_t i = 0; i < buffer.count; i++) {
        get_complex_sample(sample);
        buffer.p[i].i = sample.i;
        buffer.p[i].r = sample.r;
    }
}
} // namespace dsp
