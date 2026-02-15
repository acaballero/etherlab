//
// Created by Angel Dust on 03/07/2024.
//

#ifndef TRX_FRONTEND_OOK_MODULATOR_H
#define TRX_FRONTEND_OOK_MODULATOR_H

#include "dsp/dsp_common.h"
#include "output.h"

namespace dsp {
class OOKModulator : public Output<complex_t> {

  public:
    OOKModulator(Output *mod, Output *base) : modulation{mod}, baseband{base} {};

    void get_block(buffer_t<complex_t> &buff) override;
    void get_sample(adc_type &sample) override;
    void get_complex_sample(complex_t &sample) override;

  protected:
    Output *modulation;
    Output *baseband;
};
} // namespace dsp

#endif // TRX_FRONTEND_OOK_MODULATOR_H
