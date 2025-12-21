//
// Created by Angel Dust on 19/12/2024.
//

#ifndef TRX_FRONTEND_MODULATOR_H
#define TRX_FRONTEND_MODULATOR_H

#include "dsp/dsp_common.h"
#include "output.h"
#include <sys/_stdint.h>

class Modulator : public Output<complex_t> {

  public:
    Modulator(Output *mod, Output *base) : modulation{mod}, baseband{base} {};

    void set_modulation(Output *m) {
        modulation = m;
    };
    void set_baseband(Output *m) {
        baseband = m;
    };

    void set_modulation_offset(int16_t v) {
        modulation_offset = v;
    };

    void get_block(buffer_t<complex_t> &buff) override;
    void get_sample(adc_type &sample) override;
    void get_complex_sample(complex_t &sample) override;

  protected:
    Output *modulation;
    Output *baseband;
    int16_t modulation_offset = 0;
};

#endif // TRX_FRONTEND_OOK_MODULATOR_H
