//
// Created by Angel Dust on 11/12/2022.
//

#ifndef TRX_FRONTEND_DC_BLOCK_H
#define TRX_FRONTEND_DC_BLOCK_H

#include <stdio.h>
#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"

/*
 * A DC blocker IIR filter
 *
 * This may be implemented with a generic pole/zero filter setting a zero at Z=1 and a pole at z close to 1, but here we make some optimizations
 */
class DCBlock {

  public:
    explicit DCBlock() : pole_radius{0.999} {
    }
    explicit DCBlock(float pole_radius) : pole_radius{pole_radius} {
    }
    int16_t filter(const int16_t sample);
    float32_t filter(const float32_t input);
    void filter(const buffer_t<int16_t> &src, uint8_t n_channels, uint8_t channel);
    void filter(const buffer_t<float32_t> &src, uint8_t n_channels, uint8_t channel_n);

  private:
    float32_t pole_radius;
    float32_t last_x = 0;
    float32_t last_y = 0;
};

#endif // TRX_FRONTEND_DC_BLOCK_H
