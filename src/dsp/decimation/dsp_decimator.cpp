
#include "dsp_decimator.h"


template<typename T>
uint16_t DspDecimator<T>::getFactor() const {
    return factor;
}

template<typename T>
void DspDecimator<T>::setFactor(uint16_t factor) {
    DspDecimator::factor = factor;
}

template<typename T>
uint32_t DspDecimator<T>::getInputRate() const {
    return input_rate;
}

template<typename T>
void DspDecimator<T>::setInputRate(uint32_t inputRate) {
    input_rate = inputRate;
}

template<typename T>
uint32_t DspDecimator<T>::getOutputRate() const {
    return output_rate;
}

template<typename T>
void DspDecimator<T>::setOutputRate(uint32_t outputRate) {
    output_rate = outputRate;
}
