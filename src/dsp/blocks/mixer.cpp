//
// Created by Angel Dust on 03/07/2024.
//

#include "mixer.h"

void Mixer::get_block(buffer_t<complex_t> &buffer) {

    complex_t sample;

    for (size_t i = 0; i < buffer.count; i++) {

        get_complex_sample(sample);

        buffer.p[i].r = sample.r;
        buffer.p[i].i = sample.i;
    }
}

void Mixer::get_complex_sample(complex_t &sample) {

    complex_t sample_lo, sample_rf;
    lo->get_complex_sample(sample_lo);
    rf->get_complex_sample(sample_rf);

    sample.r = sample_lo.r>0 ? sample_rf.r : -sample_rf.r;
    sample.i = sample_lo.i>0 ? sample_rf.i : -sample_rf.i;
}
