//
// Created by Angel Dust on 19/12/2024.
//

#include "modulator.h"

void Modulator::get_block(buffer_t<complex_t> &buffer) {

    complex_t sample;

    for (size_t i = 0; i < buffer.count; i++) {
        get_complex_sample(sample);
        buffer.p[i].r = sample.r;
        buffer.p[i].i = sample.i;
    }
}

void Modulator::get_sample(adc_type &sample) {
    adc_type sample_base;
    adc_type sample_mod;
    modulation->get_sample(sample_mod);
    baseband->get_sample(sample_base);

    sample_mod += modulation_offset;

    sample = (((float)sample_mod / (0x7FF + modulation_offset)) * sample_base);
}

void Modulator::get_complex_sample(complex_t &sample) {

    complex_t sample_base;
    adc_type sample_mod;
    modulation->get_sample(sample_mod);
    baseband->get_complex_sample(sample_base);

    sample_mod += modulation_offset;

    sample.r = (((float)sample_mod / (0x7FF + modulation_offset)) * sample_base.r);
    sample.i = (((float)sample_mod / (0x7FF + modulation_offset)) * sample_base.i);
}
