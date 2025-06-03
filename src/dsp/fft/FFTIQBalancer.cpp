//
// Created by Angel Dust on 16/11/2019.
//

#include <algorithm>
#include <string.h>
#include "FFTIQBalancer.h"
#include <math.h>
#include "fft.h"

#define SQR(a) (a * a)

FFTIQBalancer::FFTIQBalancer() {

    this->correctionEnabled = true;
    this->estimationEnabled = true;

    for (int i = 0; i < FFT_IQ_BALANCER_FILTER_SIZE; i++) {
        this->gainPoints[i] = 1;
    }
}

BalanceEstate FFTIQBalancer::estimate(complex_t_f32 *fft) {

    this->changed = false;

    /*
    complex_t_f32 data[FFT_N];
    memcpy(data, fft, FFT_N * sizeof(*fft));
    reorderBins(data);
     */

    if (this->estimationEnabled) {

        this->collectBalanceInfo(fft);

        if (true) { // inteval ms elapsed?

            this->changed = this->rebuildFilter();
            this->leak();
        }
    }

    return this->state;
}

BalanceEstate FFTIQBalancer::correct(complex_t_f32 *data) {

    if (this->correctionEnabled) {
        this->correctSpectrum(data);
    }

    return this->state;
}

void FFTIQBalancer::reset() {

    memset(this->meanZ, 0, FFT_IQ_BALANCER_FILTER_SIZE * sizeof(complex_t_f32));
    memset(this->precZ, 0, FFT_IQ_BALANCER_FILTER_SIZE * sizeof(complex_t_f32));
}

bool FFTIQBalancer::rebuildFilter() {

    bool ret = false;
    // float32_t x, gain, phase;

    if (this->fitPolynomial()) {

        for (int i = 0; i < FFT_IQ_BALANCER_FILTER_SIZE; i++) {

            /* Using interpolated gain and phase points
            x = this->polyX(i);
            gain = this->coeffG[0];

            for (int p = 1; p < FFT_N; p++) {
                gain += this->coeffG[p] * 1;//intPower(x, p);
            }

            phase = this->coeffF[0];

            for (int p = 1; p < FFT_N; p++) {
                phase += this->coeffF[p] * 1;//intPower(x, p);
            }


            filter[i].r = gain * cos(phase) / FFT_N;
            filter[i].i = gain * sin(phase) / FFT_N;

              */

            // Using obtained gain and phase points

            filter[i].r = (double)this->gainPoints[i] * (double)cos((double)this->phasePoints[i]); // / FFT_N;
            filter[i].i = (double)this->gainPoints[i] * (double)sin((double)this->phasePoints[i]); // / FFT_N;
        }

        /*
                for (int i = 0; i < FFT_N; i++) {
                    filter[i].i = 0;
                    filter[i].r = 0;
                }


                int n = 41;
                for (int i = 0; i < n; i++) {

                    filter[FFT_N / 2 - n / 2 + i].r = 1.0;/// (float) FFT_N;

                }
        */

        // The bins need to be in [DC,positive freqs,negative freqs] order before taking the FFT or IFFT
        // reorderBins(filter);

        // Transform to time domain
        //  (*arm_cfft)(&S_cfft, (float32_t *) filter, 1, 1);

        // Our window function is DC centered, so we have to reorder the bins back to [negative freqs,DC,positive freqs]
        //    reorderBins(filter);

        // Apply window
        //  arm_cmplx_mult_real_f32((float32_t *) filter, window, (float32_t *) filter, FFT_N);
        /*
                lcd.clear(1,false);
                lcd.setBuffer(1);
                drawCmplx_f32((float32_t *)filter,FFT_N,0);
                lcd.renderBuffer(1);
                HAL_Delay(1000);
                */

        // Back to frequency domain
        // The frequency response is now interpolated over the whole FFT size
        // (*arm_cfft)(&S_cfft, (float32_t *) filter, 0, 1);

        // reorderBins(filter);

        /*
                lcd.clear(1,false);
                drawFFT((float32_t *) filter,0,FFT_N,FFT_N,2,false);
                lcd.renderBuffer(1);
                HAL_Delay(1000);
        */
        this->state = BS_OK;
        ret = true;
    }

    return ret;
}

void FFTIQBalancer::correctSpectrum(complex_t_f32 *data) {

    complex_t_f32 sp, sm, err;

    complex_t_f32 copy[FFT_N];
    memcpy(copy, data, FFT_N * sizeof(*data));

    int j, fi = 0;

    if (this->state == BS_OK) {

        for (int i = 0; i < FFT_N; i++) {

            j = (FFT_N - i) % FFT_N; // modulus helps when i=0;

            sp = copy[i];
            sm = copy[j];

            // Map FFT bin to filter bin
            // The relation between the resolution bandwidths of the filter and the FFT is used to map
            // each FFT bin to its corresponding bin in the filter.
            // filter_rbw should be greater than fft_rbw to be able to map all FFT frequencies to the filter
            fi = mapBin(j);

            if (fi >= 0 && fi < FFT_IQ_BALANCER_FILTER_SIZE) {

                err = filter[fi];

                data[i].r = (sp.r + sm.r - (sp.i + sm.i) * err.i + (sp.r - sm.r) * err.r) / 2;
                data[i].i = (sp.i - sm.i + (sp.i + sm.i) * err.r + (sp.r - sm.r) * err.i) / 2;
            }
        }
    }
}

bool FFTIQBalancer::fitPolynomial() {

    bool ret = false;

    // Interpolate values near DC and join the extremes

    // NOTE: The quadrature mixer/ADC filter alone usually (and tested in the current board) has a linear response
    // in both the amplitude and phase mismatches. The amplitude mismatch graph has the form of a dome, increasing
    // towards DC and decreasing as frequency increase. The phase mismatch graph has the shape of an 'S', but rotated
    // 90º, meaning the phase mismatch inverts its value as it crosses DC and increases (in absolute value) with frequency.
    // This linearity should ease the interpolation of points wherever we cannot calculate the values.
    //
    // However, when placing the IF buffer amplifier and low pass filter before the quadrature mixer, both amplitude
    // and phase imbalances change their shapes in such a way that they change signs near DC (I think it's caused by the
    // phase response of the low pass filter after the buffer and before the mixer), and this is making the naïve interpolation
    // I'm using near useless, causing a very bad I/Q balancing near DC.

    uint16_t imin = 0, imax = FFT_IQ_BALANCER_FILTER_SIZE - 1; // Min and max2 FFT indices which have precision. Used to interpolate both ends

    uint16_t si = (FFT_IQ_BALANCER_FILTER_SIZE >> 1) - this->centerBins;
    uint16_t ei = si + (this->centerBins << 1) + 1;
    complex_t_f32 sm = meanZ[si - 1];
    complex_t_f32 em = meanZ[ei + 1];
    uint16_t i;

    for (int i = si; i < ei; i++) {

        meanZ[i].r = (sm.r * (ei + 1 - i) + em.r * (i - si + 1)) / (ei + 1 - si + 1);
        meanZ[i].i = (sm.i * (ei + 1 - i) + em.i * (i - si + 1)) / (ei + 1 - si + 1);
        precZ[i] = FFT_IQ_BALANCER_MIN_PRECISSION;
    }

    // Search first and last non-zero gains
    while (imax && this->precZ[imax] <= FFT_IQ_BALANCER_MIN_PRECISSION) {
        imax--;
    }

    while (imin < imax && this->precZ[imin] <= FFT_IQ_BALANCER_MIN_PRECISSION) {
        imin++;
    }

    if (imin < imax) {

        sm = meanZ[imax];
        em = meanZ[imin];

        ei = FFT_IQ_BALANCER_FILTER_SIZE + imin;
        si = imax + 1;
        float dx = (ei + 1 - si + 1);

        for (int ix = si; ix < ei; ix++) {

            i = ix % FFT_IQ_BALANCER_FILTER_SIZE;

            meanZ[i].r = (sm.r * (ei + 1 - ix) + em.r * (ix - si + 1)) / dx;
            meanZ[i].i = (sm.i * (ei + 1 - ix) + em.i * (ix - si + 1)) / dx;
            precZ[i] = FFT_IQ_BALANCER_MIN_PRECISSION;
        }
    }

    for (int i = 0; i < FFT_IQ_BALANCER_FILTER_SIZE; i++) {

        // If this point estimation is reliable
        if (this->precZ[i] >= FFT_IQ_BALANCER_MIN_PRECISSION) {

            complex_t_f32 z = this->meanZ[i];

            if (abs(z.r) <= 0.5) { // Reject spikes

                float32_t den = 0, phase = 0, gain = 0;

                arm_sqrt_f32(1.0f - SQR(2.0f * z.r), &den);

                // Phase and gain of the detected image signal
                phase = asin(2.0f * z.i / den);
                gain = den / (1.0f - 2.0f * z.r);

                // Keep the phase and gain mismatch if they are less than 20º and 1dB (otherwise they may be false positives)
                if ((abs(phase * 180 / PI) <= 30) && abs(fasterlog(gain + 1e-30)) <= 1) {

                    // Valid estimate
                    this->gainPoints[i] = gain;
                    this->phasePoints[i] = phase;
                }
            }
        }
    }

    ret = true; // TODO: Always true for the moment

    return ret;
}

float32_t *FFTIQBalancer::getGainPoints() {
    return this->gainPoints;
}

float32_t *FFTIQBalancer::getPhasePoints() {
    return this->phasePoints;
}

float32_t *FFTIQBalancer::getPrecisionPoints() {
    return this->precZ;
}

complex_t_f32 *FFTIQBalancer::getMeanPoints() {
    return this->meanZ;
}

complex_t_f32 *FFTIQBalancer::getFilter() {
    return this->filter;
}

void FFTIQBalancer::setFilter(complex_t_f32 *v) {
    memcpy(this->filter, v, FFT_IQ_BALANCER_FILTER_SIZE * sizeof(complex_t_f32));
}

void FFTIQBalancer::setPrecZ(float32_t *v) {
    memcpy(this->precZ, v, FFT_IQ_BALANCER_FILTER_SIZE * sizeof(float32_t));
    this->rebuildFilter();
}

void FFTIQBalancer::setMeanZ(complex_t_f32 *v) {

    memcpy(this->meanZ, v, FFT_IQ_BALANCER_FILTER_SIZE * sizeof(complex_t_f32));
    this->rebuildFilter();
}

int16_t FFTIQBalancer::mapBin(uint16_t i) {

    uint16_t m = (FFT_N >> 1);
    uint16_t mf = (FFT_IQ_BALANCER_FILTER_SIZE >> 1);
    uint16_t d = abs(m - i);
    uint16_t df = d * fft_rbw / filter_rbw_hz;

    return i >= m ? mf + df : mf - df;
}

void FFTIQBalancer::collectBalanceInfo(complex_t_f32 *data) {

#if DEBUG_FFT_IQ_BALANCER

    // printf("FFTIQBalancer::collectBalanceInfo - FFT data:\n");
    //  print_vector_complex_f32((float32_t *)data,FFT_N);

#endif

    //   uint8_t shrink = FFT_N / FFT_IQ_BALANCER_FILTER_SIZE;
    int16_t j = 0;

    // Margin bins outside the bandwidth of interest of the whole FFT

    uint8_t marginBins = fft_params.start_bin;

    // Calculate SNR squared

    float32_t snrSqr[FFT_N];
    for (int i = 0; i < FFT_N; i++) {

        snrSqr[i] = SQR(data[i].r) + SQR(data[i].i);
    }

    float32_t copy[FFT_N];
    memcpy(copy, snrSqr, FFT_N * sizeof(*snrSqr));

    std::sort(copy, copy + FFT_N);

    this->noise_level = copy[FFT_N >> 1];
    // arm_mean_f32((float32_t *)snrSqr,FFT_N,&snrMedian); // Mean

    for (int i = 0; i < FFT_N; i++) {

        snrSqr[i] = snrSqr[i] / this->noise_level;
    }

#if DEBUG_FFT_IQ_BALANCER

    printf("FFTIQBalancer::collectBalanceInfo - SNR squared:\n");
    print_vector_f32(snrSqr, FFT_N);

#endif

    // Find peaks
    for (int i = marginBins; i < FFT_N - marginBins; i++) {

        if ((abs(i - (FFT_N >> 1))) > this->centerBins) {

            if (snrSqr[i] > FFT_IQ_BALANCER_MIN_SNR_SQ && snrSqr[FFT_N - i] > 0 && (snrSqr[i] / snrSqr[FFT_N - i] > 10)) {

                // Synchronous detection of the image using the main signal as a reference oscillator
                // The synchronous detector has very high sensitivity and can detect the image signal even if it is below the noise.
                // z is the normalized output power of the image
                complex_t_f32 z = complexMult(data[i], data[FFT_N - i]);
                float32_t pwr = SQR(data[i].r) + SQR(data[i].i) + SQR(data[FFT_N - i].r) + SQR(data[FFT_N - i].i);
                z.r = z.r / pwr;
                z.i = z.i / pwr;

                // Precision of observation
                float32_t precision = 1e-6 * (snrSqr[i] + snrSqr[FFT_N - i]);

                if (precision < 1e10) {

                    // Map FFT bin to filter bin
                    j = mapBin(i);

                    if (j >= 0 && j < FFT_IQ_BALANCER_FILTER_SIZE) {

                        precZ[j] = precZ[j] + precision;
                        precZ[j] = min2(precZ[j], FFT_IQ_BALANCER_MAX_PRECISSION);

                        // Weighted mean over time of the detected image
                        z.r = (meanZ[j].r * (precZ[j] - precision) + z.r * precision) / precZ[j];
                        z.i = (meanZ[j].i * (precZ[j] - precision) + z.i * precision) / precZ[j];

                        meanZ[j] = z;
                    }
                }
            }
        }
    }

#if DEBUG_FFT_IQ_BALANCER

    printf("\nFFTIQBalancer::collectBalanceInfo - mean:\n");
    print_vector_complex_f32((float32_t *)this->meanZ, FFT_N);
    printf("\nFFTIQBalancer::collectBalanceInfo - precision:\n");
    print_vector_f32(this->precZ, FFT_N);

#endif
}

void FFTIQBalancer::leak() {

    float energy = 0;

    for (int i = 0; i < FFT_N; i++) {
        energy += precZ[i];
    }

    if (energy >= FFT_IQ_BALANCER_MAX_ENERGY) {

        for (int i = 0; i < FFT_N; i++) {
            precZ[i] *= FFT_IQ_BALANCER_MAX_ENERGY / energy;
        }
    }

#if DEBUG_FFT_IQ_BALANCER

    printf("FFTIQBalancer::collectBalanceInfo - energy:\n");
    printf("%5.2f", energy);
    printf("FFTIQBalancer::collectBalanceInfo - precision:\n");
    print_vector_f32((float *)this->precZ, FFT_N);

#endif
}

/* map index (0..filtersize) to -0.5..0.5
the negative part is in the upper half of the array
 */
float FFTIQBalancer::polyX(int) {

    // TODO:
    return 0;
}

bool FFTIQBalancer::isEstimationEnabled() const {
    return estimationEnabled;
}

void FFTIQBalancer::setEstimationEnabled(bool estimationEnabled) {
    FFTIQBalancer::estimationEnabled = estimationEnabled;
}

bool FFTIQBalancer::isCorrectionEnabled() const {
    return correctionEnabled;
}

void FFTIQBalancer::setCorrectionEnabled(bool correctionEnabled) {
    FFTIQBalancer::correctionEnabled = correctionEnabled;
}

float FFTIQBalancer::getFilterRbw() const {
    return filter_rbw_hz;
}

void FFTIQBalancer::setFilterRbw(float filterRbw) {
    filter_rbw_hz = filterRbw;
}

float FFTIQBalancer::getFftRbw() const {
    return fft_rbw;
}

void FFTIQBalancer::setFftRbw(float fftRbw) {
    fft_rbw = fftRbw;
}

float FFTIQBalancer::getNoiseLevel() const {
    return noise_level;
}

void FFTIQBalancer::init() {
}
