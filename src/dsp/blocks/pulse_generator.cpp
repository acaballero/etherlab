//
// Created by Angel Dust on 02/07/2024.
//

#include "pulse_generator.h"
#include "config.h"
#include "blocks_common.h"

void PulseGenerator::init() {
    tone_delta = (uint32_t)(((float)(LUT_SIZE * frequency) / (float)sample_rate) * (1 << 24));
    crossover_phase = (uint8_t)((float)(LUT_SIZE - 1) * (float)duty / 100.0f);
}

adc_type PulseGenerator::get_sample(uint32_t phase) {

    adc_type sample;

    sample = (((phase & 0xFF000000) >> 24) <= crossover_phase) ? 2047 : -2048;

    return sample;
}

void PulseGenerator::get_sample(adc_type &sample) {
    tone_phase += tone_delta;
    sample = get_sample(tone_phase);
}

void PulseGenerator::get_complex_sample(complex_t &sample) {

    //        if (!sample_count && auto_off) {
    //            txprogress_message.done = true;
    //            shared_memory.application_queue.push(txprogress_message);
    //        } else
    //            sample_count--;

    tone_phase += tone_delta;

    sample.r = get_sample(tone_phase);
    sample.i = get_sample(tone_phase + ((LUT_SIZE >> 2) << 24)); // 90 deg
}

void PulseGenerator::get_block(buffer_t<complex_t> &buffer) {
    complex_t sample;
    for (size_t i = 0; i < buffer.count; i++) {
        get_complex_sample(sample);
        buffer.p[i].i = sample.i;
        buffer.p[i].r = sample.r;
    }
}

void PulseGenerator::set_duty(uint8_t d) {
    duty = d;
    init();
}

void PulseGenerator::set_config(uint32_t f, uint32_t sr) {
    frequency = f;
    sample_rate = sr;
    init();
}
