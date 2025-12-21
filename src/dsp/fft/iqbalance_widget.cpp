//
// Created by Angel Dust on 18/04/2021.
//

#include "iqbalance_widget.h"
#include "fft.h"
#include "FFTIQBalancer.h"
#include "../../../lib/utils/utils.hpp"

bool IQBalanceWidget::paint_callback() {

    float phaseY, gainY = 0;

    // display->setBuffer(2);
    display->clear();

    uint16_t px = 0;

    // Pixel width to draw the interpolated FFT_N points over the whole width of the display
    uint8_t bar_w = DISPLAY_X_PIXELS / FFT_N;

    float32_t *phasePoints, *gainPoints, *precPoints;
    phasePoints = fft_iq_balancer.getPhasePoints();
    gainPoints = fft_iq_balancer.getGainPoints();
    precPoints = fft_iq_balancer.getPrecisionPoints();

    float32_t maxGain = 0, maxPhase, minGain = 0.7, minPhase = -0.1;

    min_max_f32(gainPoints, FFT_N, &minGain, &maxGain, 0);
    min_max_f32(phasePoints, FFT_N, &minPhase, &maxPhase);

    // Y scale
    float32_t phaseAmp = maxPhase - minPhase + 1e-5;
    float32_t gainAmp = maxGain - minGain + 1e-5;

    // Make some headroom (5%) above and below
    minPhase -= phaseAmp * 0.05;
    minGain -= gainAmp * 0.05;

    phaseAmp *= 1.1;
    gainAmp *= 1.1;

    uint16_t h = this->size().height();

    if (maxGain) {
        for (uint16_t i = 0; i < FFT_IQ_BALANCER_FILTER_SIZE; i++) {

            phaseY = ((phasePoints[i] - minPhase) / phaseAmp) * h;
            gainY = ((gainPoints[i] - minGain) / gainAmp) * h;

            uint16_t ex = px + bar_w;

            while (px < ex) {
                display->setPixel(px, h - gainY, precPoints[i] > FFT_IQ_BALANCER_MIN_PRECISSION ? C565_WHITE : C565_RED);
                display->setPixel(px, h - phaseY, precPoints[i] > FFT_IQ_BALANCER_MIN_PRECISSION ? C565_CYAN : C565_BLUE);
                px++;
            }
        }
    }

    return true;
}

void IQBalanceWidget::before_paint() {
}
