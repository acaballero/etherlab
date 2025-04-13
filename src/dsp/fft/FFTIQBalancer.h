//
// Created by Angel Dust on 16/11/2019.
//

#ifndef TRX_FRONTEND_FFTIQBALANCER_H
#define TRX_FRONTEND_FFTIQBALANCER_H

#include "fft_types.h"
#include "dsp/dsp_common.h"

#define DEBUG_FFT_IQ_BALANCER 0

enum BalanceEstate { BS_NODATA, BS_ROUGH, BS_OK };

#define FFT_IQ_BALANCER_FILTER_SIZE FFT_N
#define FFT_IQ_BALANCER_MIN_PRECISSION 1
#define FFT_IQ_BALANCER_MAX_PRECISSION 1e9
#define FFT_IQ_BALANCER_MAX_ENERGY 1e10
#define FFT_IQ_BALANCER_MIN_SNR_SQ 1000 // 30dB

class FFTIQBalancer {

  public:
    FFTIQBalancer();

    float32_t *getPhasePoints();
    float32_t *getGainPoints();
    complex_t_f32 *getMeanPoints();
    float32_t *getPrecisionPoints();
    complex_t_f32 *getFilter();
    void setFilter(complex_t_f32 *);
    void setPrecZ(float32_t *);
    void setMeanZ(complex_t_f32 *);

    bool isEstimationEnabled() const;

    void setEstimationEnabled(bool estimationEnabled);

    bool isCorrectionEnabled() const;

    void setCorrectionEnabled(bool correctionEnabled);

    float getFilterRbw() const;

    void setFilterRbw(float filterRbw);

    float getFftRbw() const;

    void setFftRbw(float fftRbw);

    void reset();

    BalanceEstate estimate(complex_t_f32 *data);
    BalanceEstate correct(complex_t_f32 *data);

    void init();

    float getNoiseLevel() const;

  private:
    bool estimationEnabled;
    bool correctionEnabled;

    // Resolution bandwidth. Bandwidth of each point in the filter
    // **Note** that, if we change the filter_rbw, the filter's saved data in the EEPROM won't be valid
    float filter_rbw_hz = 3000;

    // Noise level (calculated as the median value of the FFT)
    float noise_level;

    // Resolution bandwidth of the FFT.
    // The relation between the resolution bandwidths of the filter and the FFT is used to map
    // each FFT bin to its corresponding bin in the filter.
    // filter_rbw should be greater than fft_rbw to be able to map all FFT frequencies to the filter
    // TODO: Allow for the size of the filter to be greater than FFT_N to be able to set filter_rbw less than fft_rbw
    float fft_rbw = 1;

    // number of bins near DC for which we don't estimate the imbalance
    uint8_t centerBins = 1;

    BalanceEstate state = BS_NODATA;

    bool changed;

    complex_t_f32 filter[FFT_IQ_BALANCER_FILTER_SIZE];

    float32_t gainPoints[FFT_IQ_BALANCER_FILTER_SIZE], phasePoints[FFT_IQ_BALANCER_FILTER_SIZE];

    // Synchronous detector output
    complex_t_f32 meanZ[FFT_IQ_BALANCER_FILTER_SIZE];
    float32_t precZ[FFT_IQ_BALANCER_FILTER_SIZE];

    void collectBalanceInfo(complex_t_f32 *data);

    bool rebuildFilter();

    void correctSpectrum(complex_t_f32 *data);

    float polyX(int i);

    bool fitPolynomial();

    void leak();

    int16_t mapBin(uint16_t);
};

#endif // TRX_FRONTEND_FFTIQBALANCER_H
