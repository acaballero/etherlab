//
// Created by Angel Dust on 20/01/2024.
//

#ifndef TRX_FRONTEND_MIXER_H
#define TRX_FRONTEND_MIXER_H

#include "hw/stm32.h"

enum LO_INJECTION {
    LOW_SIDE = -1, HIGH_SIDE = 1, ANY_SIDE = 0
};

class mixer {

public:
    uint64_t getRf();
    uint64_t calcRf();
    void setRf(uint64_t fRf);

    uint64_t getLo();
    uint64_t calcLo();
    void setLo(uint64_t fLo);

    uint64_t getIf();
    uint64_t calcIf();
    void setIf(uint64_t fIf);

    LO_INJECTION getLoInjection() const;

    void setLoInjection(LO_INJECTION injection_side);


private:
    uint64_t f_rf = 0;
    uint64_t f_lo = 0;
    uint64_t f_if = 0;
    LO_INJECTION lo_injection = LOW_SIDE;
};

#endif //TRX_FRONTEND_MIXER_H
