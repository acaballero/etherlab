//
// Created by Angel Dust on 19/12/2024.
//

#ifndef TRX_FRONTEND_MODULATOR_H
#define TRX_FRONTEND_MODULATOR_H

#include "dsp/dsp_common.h"
#include "output.h"
#include <sys/_stdint.h>

namespace dsp {
class Modulator : public Output<complex_t> {

  public:
    Modulator(Signal<> *mod, Signal<> *base) : modulation{mod}, baseband{base} {};

    void set_modulation(Signal<> *m) {
        modulation = m;
    };
    void set_baseband(Signal<> *m) {
        baseband = m;
    };

    void set_modulation_offset(int16_t v) {
        modulation_offset = v;
    };

    void get_block(buffer_t<complex_t> &buff) override;
    void get_sample(adc_type &sample) override;
    void get_complex_sample(complex_t &sample) override;

  protected:
    Signal<> *modulation;
    Signal<> *baseband;

    // Common mode of the modulation signal
    // The signals are expected to be of 12-bit precission. With 0 common mode, that is for -0x800 to 0x7FF
    // If the signal is unsinged (0 to 0xFFF), use a modulation offset of 0x7FF
    int16_t modulation_offset = 0;
};

#endif // TRX_FRONTEND_OOK_MODULATOR_H
}
