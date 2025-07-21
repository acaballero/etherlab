
#include "dsp_decimator.h"
#include "dsp/dsp_common.h"

template class DspDecimator<uint16_t>;
template class DspDecimator<complex_t_f32>;
template class DspDecimator<short>;

template <typename T> void DspDecimator<T>::set_factor(uint16_t factor) {
    DspDecimator::factor = factor;
}

template <typename T> uint32_t DspDecimator<T>::get_input_rate() const {
    return input_rate;
}

template <typename T> void DspDecimator<T>::set_input_rate(uint32_t inputRate) {
    input_rate = inputRate;
}

template <typename T> uint32_t DspDecimator<T>::get_bandwidth() const {
    return bandwidth;
}

template <typename T> void DspDecimator<T>::set_bandwidth(uint32_t outputRate) {
    bandwidth = outputRate;
}
