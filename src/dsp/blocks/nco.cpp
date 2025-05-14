//
// Created by Angel Dust on 15/05/2025.
//

#include "nco.h"
#include "arm_math.h"

void NCO::get_block(buffer_t<complex_t_f32> &buffer) {
    complex_t_f32 sample;

    for (size_t i = 0; i < buffer.count; i++) {
        get_complex_sample(sample);
        buffer.p[i].r = sample.r;
        buffer.p[i].i = sample.i;
    }
}
void NCO::get_complex_sample(complex_t_f32 &sample) {

    arm_sin_cos_f32(phase, &sample.r, &sample.i);
}

void NCO::get_sample(float32_t &) {
    // TODO: Does this make sense?
}
