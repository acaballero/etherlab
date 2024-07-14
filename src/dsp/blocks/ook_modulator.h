//
// Created by Angel Dust on 03/07/2024.
//

#ifndef TRX_FRONTEND_OOK_MODULATOR_H
#define TRX_FRONTEND_OOK_MODULATOR_H


#include "output.h"

class OOKModulator : public Output {

public:

    OOKModulator(Output *mod, Output *base) : modulation{mod}, baseband{base} {};

    void set_dc_offset(adc_type offset) { dc_offset = offset; };

    void get_block(buffer_t<complex_t> &buff) override;
    void get_sample(adc_type &sample) override;
    void get_complex_sample(complex_t &sample) override;

protected:
    Output *modulation;
    Output *baseband;
    adc_type dc_offset;
};


#endif //TRX_FRONTEND_OOK_MODULATOR_H
