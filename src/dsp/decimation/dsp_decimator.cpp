
#include "dsp_decimator.h"

template class DspDecimator<uint16_t>;
template class DspDecimator<short>;

template <typename T> void DspDecimator<T>::set_factor(uint16_t factor) {
    DspDecimator::factor = factor;
}

template <typename T> uint32_t DspDecimator<T>::getInputRate() const {
    return input_rate;
}

template <typename T> void DspDecimator<T>::setInputRate(uint32_t inputRate) {
    input_rate = inputRate;
}

template <typename T> uint32_t DspDecimator<T>::getBandwidth() const {
    return bandwidth;
}

template <typename T> void DspDecimator<T>::setBandwidth(uint32_t outputRate) {
    bandwidth = outputRate;
}
