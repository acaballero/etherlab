//
// Created by Angel Dust on 20/01/2024.
//

#include "mixer.h"
#include <cmath>

uint64_t mixer::calcRf() {
    f_rf = f_if + f_lo;
    return getRf();
}

uint64_t mixer::getRf() { return f_rf; }

void mixer::setRf(uint64_t fRf) { f_rf = fRf; }

uint64_t mixer::getLo() { return f_lo; }

uint64_t mixer::calcLo() {

    if (lo_injection == LOW_SIDE) {
        f_lo = std::abs((int64_t)f_if - (int64_t)f_rf);
    } else {
        f_lo = f_if + f_rf;
    }
    return getLo();
}

void mixer::setLo(uint64_t fLo) {
    f_lo = fLo;
    // lo_injection = f_lo > f_if ? HIGH_SIDE : LOW_SIDE;
}

uint64_t mixer::getIf() { return f_if; }

uint64_t mixer::calcIf() {
    if (lo_injection == LOW_SIDE) {
        f_if = std::abs((int64_t)f_lo - (int64_t)f_rf);
    } else {
        f_if = f_lo + f_rf;
    }
    return getIf();
}

void mixer::setIf(uint64_t fIf) {
    f_if = fIf;
    // lo_injection = f_lo > f_if ? HIGH_SIDE : LOW_SIDE;
}

LO_INJECTION mixer::getLoInjection() const { return lo_injection; }

void mixer::setLoInjection(LO_INJECTION injection_side) { lo_injection = injection_side; }
