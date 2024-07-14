//
// Created by Angel Dust on 03/07/2024.
//

#include "ook_modulator.h"

void OOKModulator::get_block(buffer_t<complex_t> &buffer) {

    complex_t sample;

    for (size_t i = 0; i < buffer.count; i++) {
        get_complex_sample(sample);
        buffer.p[i].r = sample.r;
        buffer.p[i].i = sample.i;
    }
}

void OOKModulator::get_sample(adc_type &sample) {
    adc_type sample_base;
    adc_type sample_mod;
    modulation->get_sample(sample_mod);
    baseband->get_sample(sample_base);

    sample = (sample_mod>0 ? sample_base : 0) + dc_offset;
}

void OOKModulator::get_complex_sample(complex_t &sample) {

    complex_t sample_base;
    adc_type sample_mod;
    modulation->get_sample(sample_mod);
    baseband->get_complex_sample(sample_base);

    sample.r = (sample_mod>0 ? sample_base.r : 0) + dc_offset;
    sample.i = (sample_mod>0 ? sample_base.i : 0) + dc_offset;
}


