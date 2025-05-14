//
// Created by Angel Dust on 03/07/2024.
//

#ifndef TRX_FRONTEND_DSP_MIXER_H
#define TRX_FRONTEND_DSP_MIXER_H

#include "signal_generator.h"

class Mixer : Output<complex_t> {

  public:
    Mixer(Output *lo, Output *rf) : lo{lo}, rf{rf} {};

    void get_block(buffer_t<complex_t> &buff) override;
    void get_complex_sample(complex_t &sample) override;

  protected:
    Output *lo;
    Output *rf;
};

#endif // TRX_FRONTEND_DSP_MIXER_H
