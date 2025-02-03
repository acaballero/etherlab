//
// Created by Angel Dust on 11/12/2022.
//

#include "dc_block.h"
#include "dsp/buffer.hpp"

inline int16_t DCBlock::filter(int16_t input) {

    last_y = input - last_x + pole_radius * last_y;
    last_x = input;
    return last_y;
}

inline float32_t DCBlock::filter(float32_t input) {
    last_y = input - last_x + pole_radius * last_y;
    last_x = input;
    return last_y;
}

void DCBlock::filter(buffer_t<int16_t> &src, uint8_t n_channels, uint8_t channel_n) {
    for (size_t i = channel_n; i < src.count; i += n_channels) {
        src.p[i] = filter(src.p[i]);
    }
}

void DCBlock::filter(buffer_t<float32_t> &src, uint8_t n_channels, uint8_t channel_n) {
    for (size_t i = channel_n; i < src.count; i += n_channels) {
        src.p[i] = filter(src.p[i]);
    }
}
