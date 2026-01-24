//
// Created by Angel Dust on 25/01/2026.
//

#include "dsp_interpolator.h"
#include "dsp/dsp_common.h"

template class DspInterpolator<uint16_t>;
template class DspInterpolator<complex_t_f32>;
template class DspInterpolator<short>;

template <typename T> void DspInterpolator<T>::set_factor(uint16_t factor) {
    DspInterpolator::factor = factor;
}

template <typename T> uint32_t DspInterpolator<T>::get_input_rate() const {
    return input_rate;
}

template <typename T> void DspInterpolator<T>::set_input_rate(uint32_t inputRate) {
    input_rate = inputRate;
}

template <typename T> uint32_t DspInterpolator<T>::get_bandwidth() const {
    return bandwidth;
}

template <typename T> void DspInterpolator<T>::set_bandwidth(uint32_t outputRate) {
    bandwidth = outputRate;
}
