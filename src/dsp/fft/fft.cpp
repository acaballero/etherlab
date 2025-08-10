//
// Created by Angel Dust on 16/10/2019.
//

#include "Display_afb.h"
#include "config.h"
#include <algorithm> // for sdt:sort
#include <arm_math.h>

#include <sys/types.h>
#include <utility>
#include "dsp/blocks/dc_block.h"
#include "arm_common_tables.h"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft.h"
#include "dsp/fft/fft_types.h"
#include "hw/stm32f4xx/adc.h"
#include "hw/stm32f4xx/timers.h"
#include "itemsTemplates.hpp"
#include "status.h"
#include "ui/view.h"
#include "fft_widget.h"
#include "fft_ui.h"
#include "dsp/window.h"
#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "radio.h"
#include "agc.h"
#include "os/periodic_task.h"
#include "ui/main_view.h"
#include "ui/view_manager.h"
#include "utils.hpp"
#include "ui/frequency_memory_ui.h"

// FFT parameters
st_fft_params fft_params;

// Current slice
uint8_t fft_slice_n;

CCM_SECTION fft_type fft_output[FFT_N];
CCM_SECTION complex_t_f32 fft_slice_buff[FFT_N];
// Wrapper over the fft_slice_vector
buffer_t<float32_t> fft_slice_buffer = {(float32_t *const)(fft_slice_buff), FFT_N * 2};

// Displayed FFT
fft_type fft_display[DISPLAY_X_PIXELS];
fft_type fft_display_db[DISPLAY_X_PIXELS];

// FFT FIFO
complex_t fft_fifo_buff[FFT_FIFO_SIZE];
FIFO fft_fifo((char *)fft_fifo_buff, FFT_FIFO_SIZE * sizeof(complex_t));

// We need a decimator for each chanel
DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS> decimator_i{};
DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS> decimator_q{};

#if FFT_N == 64

const float32_t *twiddle = twiddleCoef_64;
const uint16_t *bitRevTable = armBitRevIndexTable64;
uint16_t bitRevTableLength = ARMBITREVINDEXTABLE__64_TABLE_LENGTH;

#elif FFT_N == 128

const float32_t *twiddle = twiddleCoef_128;
const uint16_t *bitRevTable = armBitRevIndexTable128;
uint16_t bitRevTableLength = ARMBITREVINDEXTABLE_128_TABLE_LENGTH;

#elif FFT_N == 256

const float32_t *twiddle = twiddleCoef_256;
const uint16_t *bitRevTable = armBitRevIndexTable256;
uint16_t bitRevTableLength = ARMBITREVINDEXTABLE_256_TABLE_LENGTH;

#endif

// cfft instance
arm_cfft_instance_f32 S_cfft = {FFT_N, twiddle, bitRevTable, bitRevTableLength};

void (*arm_cfft)(const arm_cfft_instance_f32 *, float32_t *, uint8_t, uint8_t) = arm_cfft_f32;

void (*arm_cmplx_mag)(float32_t *pSrc, float32_t *pDst, uint32_t numSamples) = arm_cmplx_mag_f32;

// When we use multiple slices, each of them will be obtained using a different LO frequency.
// This happens to slightly change the offset at the input of the ADCs, so we use a pair of blockers (I,Q) for each slice
DCBlock dcBlockers[FFT_MAX_SLICES][2];

// Smoothing factor lookup table
// Stores 2^SMOOTH_GAIN_LUT_PRECISION values of (1 - exp(-0.005 * ((DB - NOISE_FLOOR_DB + 1))))
#define SMOOTH_GAIN_LUT_PRECISSION 4
float32_t smoothingGainLUT[1 << (SMOOTH_GAIN_LUT_PRECISSION)];
fft_type fft_peak = FFT_MIN_DB;
uint16_t fft_peak_bin = 0;
uint64_t fft_peak_f = 0;

bool first_frame = true;
// TODO: Move to storable properties
bool fft_min_db_auto = false;
bool fft_estimateIQBalance = false;
/* Noise floor calculation */
uint16_t fft_calc_noise_floor_period_ms = 200; // 0 = noise floor disabled
unsigned long fft_last_noise_floor_calculation_ms;

namespace fft {

float fft_noise_floor_db = FFT_MIN_DB; // Noise floor in dB
float snr = 1e-40f;
float dbm = FFT_MIN_DB;         // Power in the baseband (low-pass filtered)
float dbm_instant = FFT_MIN_DB; // Raw (unfiltered) power
float dbm_peak = FFT_MIN_DB;
// FFT magnitude ADC overload threshold
adc_type adc_max_ampl;

uint8_t current_max_slices = config.fft.max_slices;

void set_max_slices(uint8_t n) {

    config.fft.max_slices = n;

    if (ISANALOG) {
        current_max_slices = n;
    }
}

std::pair<int, int> get_bandwidth_pixel_range() {
    int bm_s, bm_e, bm_m;
    bm_m = DISPLAY_X_PIXELS / 2;

    int16_t px_if_width = (int16_t)(radio::get_bandwidth_hz() / fft_params.display_rbw) >> 1;
    if (config.modulation == SSB_USB) {

        bm_s = bm_m + 1;
        bm_e = bm_m + (px_if_width << 1) - 1;
    } else if (config.modulation == CW) {
        int16_t px_pitch_offset = (int16_t)(CW_PITCH_HZ / fft_params.display_rbw);
        bm_s = bm_m + px_pitch_offset - px_if_width;
        bm_e = bm_m + px_pitch_offset + px_if_width;
    } else if (config.modulation == SSB_LSB) {
        bm_s = bm_m - (px_if_width << 1) + 1;
        bm_e = bm_m - 1;
    } else {
        bm_s = bm_m - px_if_width;
        bm_e = bm_m + px_if_width;
    }

    bm_s = bm_s < 0 ? 0 : bm_s;

    return std::pair<int, int>{bm_s, bm_e};
}

void calc_snr_2() {
    const std::pair<int, int> bin_limits = get_bandwidth_pixel_range();
    const int bin_start = bin_limits.first;
    const int bin_end = bin_limits.second;
    const int start_bin = fft_params.start_bin;
    const int end_bin = start_bin + fft_params.nbins;

    float sigplusnoise = 0.0f;
    float total_signal = 0.0f;

    const float inv_ten = 0.1f;
    const float *fft_ptr = &fft_display_db[start_bin];

    // Process 4 elements at a time for better instruction pipeline usage
    int i = start_bin;
    const int unroll_end = end_bin - 3;

    for (; i < unroll_end; i += 4) {
        // Calculate 4 power values
        const float p0 = powf(10.0f, fft_ptr[0] * inv_ten);
        const float p1 = powf(10.0f, fft_ptr[1] * inv_ten);
        const float p2 = powf(10.0f, fft_ptr[2] * inv_ten);
        const float p3 = powf(10.0f, fft_ptr[3] * inv_ten);

        // Accumulate signal+noise conditionally
        if (i >= bin_start && i <= bin_end) {
            sigplusnoise += p0;
        }
        if ((i + 1) >= bin_start && (i + 1) <= bin_end) {
            sigplusnoise += p1;
        }
        if ((i + 2) >= bin_start && (i + 2) <= bin_end) {
            sigplusnoise += p2;
        }
        if ((i + 3) >= bin_start && (i + 3) <= bin_end) {
            sigplusnoise += p3;
        }

        // Accumulate total signal
        total_signal += p0 + p1 + p2 + p3;

        fft_ptr += 4;
    }

    // Handle remaining elements
    for (; i < end_bin; i++) {
        const float p = powf(10.0f, (*fft_ptr) * inv_ten);
        if (i >= bin_start && i <= bin_end) {
            sigplusnoise += p;
        }
        total_signal += p;
        fft_ptr++;
    }

    // Rest of calculation identical to original
    const float noise_floor_mag = powf(10.0f, fft_noise_floor_db * inv_ten);
    const float noise = noise_floor_mag * (float)(bin_end - bin_start + 1);
    const float signal = max2(sigplusnoise - noise, 1e-14f);
    const float curr_snr = 10.0f * fasterlog(signal / noise);

    snr = snr - 0.3f * (snr - curr_snr);

    dbm_instant = 10.0f * fasterlog(sigplusnoise);
    const float total_dbm_instant = 10.0f * fasterlog(total_signal);
    dbm = dbm - 0.3f * (dbm - dbm_instant);

    const float bandwidth_ratio = (float)radio::get_bandwidth_hz() / fft_params.span;
    const float papr_db = 3.0f + 10.0f * bandwidth_ratio;
    dbm_peak = total_dbm_instant + papr_db;
}

void calc_snr() {

    std::pair<int, int> bin_limits = get_bandwidth_pixel_range();

    float sigplusnoise = 0; // Singal plus noise in the current bandwidth
    float total_signal = 0; // Total power in the FFT

    for (int i = fft_params.start_bin; i < fft_params.start_bin + fft_params.nbins; i++) {

        float p = powf(10.0f, fft_display_db[i] / 10.0f);
        if (i >= bin_limits.first && i <= bin_limits.second) {
            // Power has to be converted to magnitude here
            sigplusnoise += p;
        }
        total_signal += p;
    }

    float noise_floor_mag = powf(10.0f, fft_noise_floor_db / 10.0f);
    float noise = noise_floor_mag * (bin_limits.second - bin_limits.first + 1);

    // Remove noise from signal (avoiding negative powers)
    float signal = max2(sigplusnoise - noise, 1e-14f);

    float curr_snr = 10.0f * fasterlog(signal / noise);

    snr = (snr - (0.3f * (snr - curr_snr)));
    dbm_instant = 10.0f * fasterlog(sigplusnoise);
    float total_dbm_instant = 10.0f * fasterlog(total_signal);
    dbm = (dbm - (0.3f * (dbm - dbm_instant)));

    float bandwidth_ratio = (float)radio::get_bandwidth_hz() / fft_params.span;
    float papr_db = 3.0f + 10.0f * bandwidth_ratio; // PAPR: Peak to average power ratio (rough estimation to avoid calculating the peak power)
    dbm_peak = total_dbm_instant + papr_db;
}

} // namespace fft

using namespace fft;

fft_type fft_peak_v = FFT_MIN_DB;

// Max FFT magnitude. Dependent on the ADC range
// float fft_range;
float fft_mag_conv_factor = V_REF / (float)FFT_N / (float)0xFFF;
float fft_radio_gain_factor;

volatile FFT_STATUS fft_status = FFT_STATUS_IDLE;
FFTIQBalancer fftIQBalancer;
float window[FFT_N];
bool initialized = false;

/* ----------- */

void fft_loop();

namespace fft {
os::periodic_task snr_task(FFT_SNR_REFRESH_PERIOD_MS, calc_snr);
os::periodic_task fft_task(config.fft.refresh_period_ms, fft_loop);
os::periodic_task iqbalance_task(FFT_IQBALANCE_REFRESH_PERIOD_MS, []() {
    view_manager::mainView.IQBalance()->set_visible(true);
    view_manager::mainView.Waterfall()->set_visible(false);
    view_manager::mainView.IQBalance()->set_dirty();
});
os::periodic_task waterfall_task(0, []() {
    view_manager::mainView.IQBalance()->set_visible(false);
    view_manager::mainView.Waterfall()->set_visible(true);
    view_manager::mainView.Waterfall()->set_dirty();
});

Signal signal{"fft_signal"};
} // namespace fft

uint64_t last_iqbalance_estimate_ms = 0;

// Calculates FFT parameters from desired span, decimation factor and n_slices
void st_fft_params::calc() {

    // Number of usable bins in each slice

    nbins = size * USABLE_BW_FACTOR;

    if (sample_freq == 0) {
        // Minimum sample frequency, taking into account the usable bandwidth of each slice
        sample_freq = span * ((float)decimation_factor / n_slices / USABLE_BW_FACTOR);
    } else {
        // Fixed sample_freq
        span = sample_freq / ((float)decimation_factor / n_slices / USABLE_BW_FACTOR);
    }

    if (n_slices == 1) {
        // Ceil to nearest factor of 19200, which is 16*1200, so the sample frequency is decimable by 16
        // and, after that, is still a multiple of 1200 bauds, which is required for   symbol synchronization in many audio processing modes
        // This can only be done when we have one slice (which is the case for real-time DSP processing)
        sample_freq = ((sample_freq + 19199) / 19200) * 19200;
    }

    // Resolution bandwidth (per FFT bin)
    rbw = sample_freq / size / decimation_factor;

    // Bandwidth per slice
    bw = span / 2 / n_slices;

    // Total bins
    total_bins = nbins * n_slices;

    // Pixels per bin
    bin_width_px = (float)total_bins / (float)DISPLAY_X_PIXELS;

    display_rbw = rbw * bin_width_px;

    slice_w_px = nbins / bin_width_px;

    // first bin to show in each slice
    start_bin = (uint8_t)((float)(size - nbins) / 2.0);

    span_if_start = radio::f_dsp_if - (span >> 1U);
    span_f_start = config.vfo[config.vfo_ix].freq - (span >> 1U);
}

bool st_fft_params::valid() {

    bool b = sample_freq >= config.fft.min_sample_rate && sample_freq <= dsp::dsp_max_sample_rate;

    // In digital mode, if near-zero tuning is active (tuning to -sample_freq/4), the available bandwidth gets reduced.
    // After shifting up again +SF/4 in software, the lower cutoff of the pre-ADC low-pass filter is brought up by
    // the same amount
    // (_______X____|___________)
    //         ^----FS/4
    //
    // .....(_______X____|______)
    //    ^-----lost
    //
    // With decimation, the bandwidht of interest is smaller and we can afford losing some phisical bandwidht, which
    // is also taken into account here
    // In the end, we need to assure that the distance from the (shifted) baseband center frequency to the lower cutoff
    // frequency of the filter is at least the bandowidth of interest (after decimation)
    b = b && ((int32_t)config.fft.bw - (ISANALOG ? 0 : abs(dsp::get_frequency_shift(sample_freq)))) >= (int32_t)bw;

    return b;
}

void fft_dcremoval(buffer_t<float32_t> &vData) {

    dcBlockers[fft_slice_n][0].filter(vData, 2, 0);
    dcBlockers[fft_slice_n][1].filter(vData, 2, 1);
}

void unzipIQSamples(complex_t_f32 *complexData, fft_type *destReal, fft_type *destImag, uint16_t size) {

    for (int i = 0; i < size; i++) {
        destReal[i] = complexData[i].r;
        destImag[i] = complexData[i].i;
    }
}
void zipIQSamples(fft_type *srcReal, fft_type *srcImag, complex_t_f32 *dest, uint16_t size) {

    for (int i = 0; i < size; i++) {
        dest[i].r = srcReal[i];
        dest[i].i = srcImag[i];
    }
}

inline float get_window_ampl_corr_factor() {
    switch (config.fft.window) {
        case FFT_WINDOW_HAMMING:
            return 1.85f;
        default:
            return 1;
    }
}

// Window ENRM (Equivalent noise resolution bandwidth)
inline float get_window_enrb_factor() {
    switch (config.fft.window) {
        case FFT_WINDOW_HAMMING:
            return 1.36f;
        default:
            return 1;
    }
}

void calcFFTRange() {

    // Values in the fft_output array are not normalized, so they are v*FFT_N where v is the voltage magnitude
    // config.fft.maxAmpl is the integer range of the ADC

    // fft_range = (float) ((config.fft.maxAmpl * FFT_N) << (FFT_SCALE_FACTOR ? (FFT_SCALE_FACTOR - 6) : 0));

    // Overload threshold
    adc_max_ampl = (float)config.fft.maxAmpl * 0.8; // FIXME: Remove the correction factor
}

/*
 * Generates the smoothing gain factors lookup table
 */
void generateSmoothingGainLUT() {

    int db = FFT_MIN_DB;
    int lut_size = (sizeof(smoothingGainLUT) / sizeof(smoothingGainLUT[0]));
    int db_step = (FFT_MAX_DB - FFT_MIN_DB) / lut_size;

    for (int i = 0; i < lut_size; i++, db += db_step) {

        smoothingGainLUT[i] = (1 - exp(-0.14 * ((db - FFT_MIN_DB + 1)))) * config.fft.smooth_factor;
    }
}

void fft_init() {

    float32_t minPrecZ, maxPrecZ;

    // Make sure the sample rate is between hardware bounds (may have been saved before)
    config.fft.min_sample_rate = max2(FFT_MIN_SAMPLE_RATE, config.fft.min_sample_rate);

    fft_task.set_period(max2(MIN_FFT_REFRESH_PERIOD, config.fft.refresh_period_ms));

    set_max_slices(config.fft.max_slices);

    min_max_f32((float32_t *)config.fft.iq_balance_precZ, FFT_IQ_BALANCER_FILTER_SIZE, &minPrecZ, &maxPrecZ);

    if (maxPrecZ > FFT_IQ_BALANCER_MIN_PRECISSION) {

        // Initialize only if we have usable data

        fftIQBalancer.setMeanZ(config.fft.iq_balance_meanZ);
        fftIQBalancer.setPrecZ(config.fft.iq_balance_precZ);
    }

    for (int i = 0; i < DISPLAY_X_PIXELS; i++) {
        fft_display[i] = FFT_HEIGHT;
    }

    fft_config(config.fft.span);

    generateSmoothingGainLUT();

    calcFFTRange();

    config.fft.max_decimation_factor = min2(config.fft.max_decimation_factor, MAX_DECIMATION_FACTOR); // sanity check

    fftUI::set_spectrum_style(config.fft.spectrum_style);
    fftUI::set_spectrum_colors(config.fft.spectrum_line_color, config.fft.spectrum_fill_color);
    fftUI::init_waterfall();
    fftUI::initIQorWaterfall();
    fft::waterfall_task.set_period(fftUI::get_waterfall_period());
}

void resetIQBalancer() {
    fftIQBalancer.reset();
}

uint32_t fft_max_span() {
    return current_max_slices * DSP_BANDWIDTH * 2;
}

/* Finds the optimal FFT parameters based on the current selected span
 *
 * Calculates:
 *
 * - Decimation factor
 * - Sample rate
 * - FFT number of usable bins
 * - Number of slides needed

 * - FFT size
 * - RBW
 * - screen pixel/bin ratio
 *
 *
 */
bool fft_config(uint32_t span) {

    uint8_t current_dec_factor = fft_params.decimation_factor;
    uint32_t current_sample_rate = config.fft.sample_rate;
    uint32_t current_bw = fft_params.bw;

    // Max span check
    uint32_t max_span = fft_max_span();
    if (span > max_span) {
        span = max_span;
    }

    st_fft_params params{.span = span};
    bool found = false;
    st_fft_params best;

    for (int s = 1; s <= current_max_slices; s++) {
        for (int d = 1; d <= config.fft.max_decimation_factor; d <<= 1) {

            params.decimation_factor = d;
            params.n_slices = s;
            params.size = FFT_N;
            params.sample_freq = 0; // calculate
            params.calc();

            if (params.valid()) {
                // Cost function is:
                // - Bin width in screen pixels: nearest to one so the bins doesn't have to be stretched nor shrink
                // - Decimation factor: the larger, the better SNR (preferred in digital RX), but also slower rates of FFT update
                if (abs(1 - params.bin_width_px) < abs(1 - best.bin_width_px) || (!ISANALOG && (params.decimation_factor > best.decimation_factor))) {
                    best = params;
                    found = true;
                }
            }
        }
    }

    if (!found) {
        params.decimation_factor = 1;
        params.n_slices = 1;
        params.size = FFT_N;
        params.sample_freq = config.fft.min_sample_rate;

        params.calc();
        best = params;

        found = true;
        // status::handleError(status::ST_ERROR, "FFT params can't be fit");
    }

    if (found) {

        fft_params = best;
        config.fft.sample_rate = fft_params.sample_freq;

        // TODO: Decimate in cascade with multiple 2M decimators instead of using bigger factors. It's way more efficient since the
        // required filter tap number increases exponentially with the order of the decimation. Plus, a 50% low pass filter has nulls in its even taps.

#if DSP_FS4_SHIFT
        if (dsp::get_freq_shift_enabled()) {
            // TODO: With more than 1 slice, the start frequency of each slice should also be shifted since bins from one slice
            // move to the adjacent slice. Not done yet.
            radio::set_dsp_frequency_shift(-dsp::get_frequency_shift());
        } else {
            radio::set_dsp_frequency_shift(0);
        }
#endif

        if (current_dec_factor != fft_params.decimation_factor) {
            // If decimation factor has changed, reset the fifo and make sure its size is a multiple
            // of the chunk size. This changes if we are decimating, since in that case, we store some filter delay blocks
            fft_fifo.set_size((FFT_N + (fft_params.decimation_factor > 1 ? (FFT_LPF_FIR_FILTER_DELAY_BLOCKS * DSP_BLOCK) : 0)) * MAX_DECIMATION_FACTOR *
                              sizeof(complex_t));
            fft_fifo.reset();
        }

        if (current_sample_rate != config.fft.sample_rate || current_bw != fft_params.bw ||
            !decimator_i.get_initialized()) { // sample frequency changed not yet initialized

            bool b = decimator_i.config(config.fft.sample_rate, fft_params.bw, fft_params.decimation_factor);
            decimator_q.config(config.fft.sample_rate, fft_params.bw, fft_params.decimation_factor);

            if (!b) {
                // Failed decimator initialization. Should't happen but we could've mess with the fft params calculation
                status::handleError(status::ST_ERROR, "Error initializing FFT decimator");
            }

            set_timer_sample_rate(ADC_DMA_TIMER, ADC_DMA_TIMER_CLOCK_HZ, config.fft.sample_rate);

            LOG("fft_config: Changed sample rate :%lu\n", config.fft.sample_rate);
            signal.emit(nullptr);
        } else {
            decimator_i.set_factor(fft_params.decimation_factor);
            decimator_q.set_factor(fft_params.decimation_factor);
        }
    }

    return found;
}

/*
 * Performance with -Og optimizations for FFT_N=128: 5ms
 */
//__attribute__((section(".ccmram")))
void doFFT() {

    //  calibrateFFT(); // To measure max2 bin value

    if (config.fft.window != FFT_WINDOW_NONE && config.fft.view_mode != FFT_VIEW_TIME_DOMAIN) {

        if (!initialized) {
            generate_window(3, window, FFT_N);
            initialized = true;
        }

        arm_cmplx_mult_real_f32((float32_t *)fft_slice_buff, window, (float32_t *)fft_slice_buff, FFT_N);
    }

    if (config.fft.view_mode == FFT_VIEW_SPECTRUM) {

        // Calculate FFT
        (*arm_cfft)(&S_cfft, (float32_t *)fft_slice_buff, 0, 1);

        reorderBins(fft_slice_buff);

        fftIQBalancer.setFftRbw(fft_params.rbw);

        if (fft_estimateIQBalance && fft_slice_n == 0) {

            fftIQBalancer.estimate(fft_slice_buff);

            complex_t_f32 *meanZ = fftIQBalancer.getMeanPoints();
            float32_t *precZ = fftIQBalancer.getPrecisionPoints();
            memcpy(config.fft.iq_balance_precZ, precZ, FFT_IQ_BALANCER_FILTER_SIZE * sizeof(config.fft.iq_balance_precZ[0]));
            memcpy(config.fft.iq_balance_meanZ, meanZ, FFT_IQ_BALANCER_FILTER_SIZE * sizeof(config.fft.iq_balance_meanZ[0]));
        }

        if (config.fft.enable_iq_balance) {
            fftIQBalancer.correct(fft_slice_buff);
        }
    }
}

void reorderBins(complex_t_f32 *v) {

    complex_t_f32 temp;
    uint16_t center_bin = FFT_N / 2;
    for (int i = 0; i < center_bin; i++) {
        temp = v[i];
        v[i] = v[center_bin + i];
        v[center_bin + i] = temp;
    }
}

uint32_t getPeak(uint32_t start_bin, uint32_t end_bin, fft_type &peak_v) {

    uint32_t max_ix = 0;

    fft_type max = -32000;

    for (uint32_t i = start_bin; i < end_bin; i++) {

        if (fft_output[i] > max) {
            max_ix = i;
            peak_v = max = fft_output[i];
        }
    }

    return max_ix;
}

/**
 * Calculate the noise floor and SNR of the FFT using the fft_display (dB) values
 * We estimate it just by taking the median, which yields good enough approximation for our needs
 * For better FFT calculation methods: https://kluedo.ub.uni-kl.de/frontdoor/deliver/index/docId/4293/file/exact_fft_measurements.pdf
 */
void calculateNoiseFloor() {

    fft_type copy[FFT_N];
    memcpy(copy, fft_output + fft_params.start_bin, fft_params.nbins * sizeof(fft_type));

    std::sort(copy, copy + fft_params.nbins);
    fft_type median = copy[fft_params.nbins >> 1];

    // Exponential filter
    fft_noise_floor_db = (fft_noise_floor_db - (0.1f * (fft_noise_floor_db - median)));

    if (fft_min_db_auto) {
        // Set the dB scale automatically
        // fft_type min_db = copy[0];

        config.fft.min_db = fft_noise_floor_db - 10;

        if (config.fft.min_db > config.fft.max_db) {
            config.fft.min_db = config.fft.max_db;
        }
    }
}

/*
 * Search for the smoothing gain factor on a precalculated lookup table
 */
float32_t getSmoothGain(float db) {

    // Some smoothing with a 1st order low pass IIR filter
    // The gain of the filter is a non-linear function of the amplitude, making the time constant high for the
    // noise (low level signals) but small (fast response) for stronger signals

    // The exp function is too slow, so we use an approximation instead
    // gain = (1 - exp(-0.005 * ((db - FFT_MIN_DB + 1))));
    // gain *= config.fft.smooth_factor;

    return smoothingGainLUT[(int)(db - FFT_MIN_DB) >> SMOOTH_GAIN_LUT_PRECISSION];
}

void testFastLog() {

    float b = 515234.0f;

    for (int i = 0; i < 2000; i++) {

        float x = i / b;
        float l1 = 20 * log10(x);
        float l2 = 20 * fasterlog(x);
        //  float e = abs(l1 - l2);

        printf("%d: L1:  %.2f L2: %.2f", i, l1, l2);
        // printf("%d: L1: %.5f L2: %.5f E: %.5f\n", i, l1, l2, e);
        HAL_Delay(10);
    }
}

/* dB-based calculation
inline fft_type fft_output_db(fft_type v) {

    float db;

    if (config.fft.window != FFT_WINDOW_NONE) {
        // Apply window amplitude correction
        v *= get_window_ampl_corr_factor();
    }

    // db referenced to the fft_range
    db = v == 0 ? config.fft.min_db : 20.0f * fasterlog((float) v / fft_range);

    // Subtract the gain
    // TODO: Take into account the AGC value (requires taking into account that the analog gain depends on frequency, filters, etc)
    db -= radio::get_gain();

    // db =  20.0 * fasterlog((float) (fft_output[ii]) / fft_amp);
    //logEvent(110,4,0);
    if (db < config.fft.min_db)
        db = FFT_MIN_DB;
    //else if (db > config.fft.max_db)
    //    db = config.fft.max_db;

    //  start = height - (uint8_t) (
    //  ((float) (fft_display[px] - config.fft.min_db) / (float) db_amp) *
    //  (float) height);

    return db;
}
*/

/*** voltage-based calculation ***/
inline fft_type fft_output_db(fft_type v) {

    float db;

    if (config.fft.window != FFT_WINDOW_NONE) {
        // Apply window amplitude correction
        v *= get_window_ampl_corr_factor();
    }

    v = v * fft_mag_conv_factor;

    // Divide by the gain
    v /= fft_radio_gain_factor;

    // dBm (account for the ENRB of the applied window) I'm assuming peak-to-peak voltage values (so dividing 400 instead of 100 to get the power from RMS)
    // because that's how I'm getting values close to what's expected, but I think this is wrong (FFT bins are voltage magnitudes)
    //
    db = 10.0f * fasterlog(1000.0f * (float)(v * v) / 400.0 / get_window_enrb_factor());

    if (db < FFT_MIN_DB) {
        db = FFT_MIN_DB;
    }
    // else if (db > config.fft.max_db)
    //     db = config.fft.max_db;

    //  start = height - (uint8_t) (
    //  ((float) (fft_display[px] - config.fft.min_db) / (float) db_amp) *
    //  (float) height);

    return db;
}

void processFFT(float32_t *v) {

    uint16_t startx;
    int8_t x_inc;

    fft_radio_gain_factor = pow(10.0, (float)agc::get_gain() / 20.0f);

    // The first pixel depends on the slice we are currently in

    if (radio::is_freq_inverted()) {

        // FFT bins are in reverse order of frequency if the mixers prior to the sampling invert the frequency
        // This happens when the number of high side LO injections is odd

        // Here, if the LO is injected in the high side, the frequency is inverted and we start drawing from right to left
        startx = DISPLAY_X_PIXELS - (uint16_t)(fft_slice_n * fft_params.slice_w_px) - 1;
        x_inc = -1;
    } else {
        startx = (uint16_t)(fft_slice_n * fft_params.slice_w_px);
        x_inc = 1;
    }

    arm_cmplx_mag_f32(v, (float32_t *)fft_output, FFT_N);

    float db = FFT_MIN_DB, db_constrained = 0;

    float gain = 1;
    uint16_t start = 0;
    uint16_t db_amp = config.fft.max_db - config.fft.min_db;
    int bin_ix, display_ix;
    float bin_pos, display_pos;
    float range_inv = 1.0 / db_amp; // Precompute division

    if (fft_params.bin_width_px < 1) {

        // Display size > number of bins => increase display index by one
        display_ix = startx;
        int last_bin_ix = fft_params.start_bin - 1;
        bin_pos = fft_params.start_bin;

        while (radio::is_freq_inverted() ? display_ix >= max2(0, startx - fft_params.slice_w_px)
                                         : display_ix < min2(DISPLAY_X_PIXELS, fft_params.slice_w_px + startx)) {
            bin_ix = uint16_t(bin_pos);

            if (bin_ix != last_bin_ix) {
                db = fft_output_db(fft_output[bin_ix]);
                fft_output[bin_ix] = db;
                gain = first_frame ? 1 : getSmoothGain(db);
                last_bin_ix = bin_ix;
            }

            db_constrained = constrain(db, config.fft.min_db, config.fft.max_db);

            // We will store fft_display in display units ('y' coordinates from the top) for the sake of speed
            // This way, we can calculate them here once instead of (like we used to do in previous versions), store it in db units and
            // calculate display coordinates in the screen drawing callbacks (it was too slow to do the math within DMA transfers)
            start = FFT_HEIGHT - (uint16_t)(((float)(db_constrained - config.fft.min_db) * range_inv) * (float)FFT_HEIGHT);

            // IIR filter
            fft_display[display_ix] = fft_display[display_ix] - (gain * (fft_display[display_ix] - (float)start));

            // Debug
            if (fft_display[display_ix] > FFT_HEIGHT) {
                fft_display[display_ix] = FFT_HEIGHT;
            }

            fft_display_db[display_ix] = fft_display_db[display_ix] - (config.fft.smooth_factor * (fft_display_db[display_ix] - db));
            display_ix += x_inc;
            bin_pos += fft_params.bin_width_px;
        }
    } else {

        // Number of bins > display size => increase bin index by one
        display_pos = startx;
        int next_display_ix = startx + x_inc;
        int nix = uint16_t(display_pos);
        bin_ix = fft_params.start_bin;
        float display_pos_incr = (1.0f / fft_params.bin_width_px) * (float)x_inc;

        while (bin_ix <= fft_params.start_bin + fft_params.nbins) {

            display_ix = nix;

            fft_output[bin_ix] = fft_output_db(fft_output[bin_ix]);

            if (db < fft_output[bin_ix]) { // Max db value among all the bins that are compressed in the current display pixel
                db = fft_output[bin_ix];
            }

            nix = uint16_t(display_pos);
            bin_ix += 1;

            if (nix == next_display_ix) { // store the accumulated value of the display

                gain = first_frame ? 1 : getSmoothGain(db);

                db_constrained = constrain(db, config.fft.min_db, config.fft.max_db);

                start = FFT_HEIGHT - (uint8_t)(((float)(db_constrained - config.fft.min_db) / (float)db_amp) * (float)FFT_HEIGHT);

                // IIR filter
                fft_display[display_ix] = fft_display[display_ix] - (gain * (fft_display[display_ix] - (float)start));
                fft_display_db[display_ix] = fft_display_db[display_ix] - (config.fft.smooth_factor * (fft_display_db[display_ix] - db));

                next_display_ix += x_inc;
                db = FFT_MIN_DB;
            }

            display_pos += display_pos_incr;
        }

        fft_display[nix] = fft_display[nix] - (gain * (fft_display[nix] - (float)start));
        fft_display_db[nix] = fft_display_db[nix] - (gain * (fft_display_db[nix] - db));
    }

    // If the slice is not fully shown, set the fft_output to min_db so they're not used in further calculations (getPeak, for example)
    /*while (ii < fft_params.start_bin + fft_params.nbins) {
        //fft_output[ii] = config.fft.min_db;
        fft_output[ii] = FFT_MIN_DB;
        ii++;
    }*/

    // Calculate noise floor if needed
    if (fft_calc_noise_floor_period_ms > 0) {
        unsigned long ms = HAL_GetTick();
        if (ms - fft_last_noise_floor_calculation_ms > fft_calc_noise_floor_period_ms) {
            calculateNoiseFloor();
            fft_last_noise_floor_calculation_ms = ms;
        }
    }

#if DEBUG_FFT_ADC
    printf("Power spectrum\r\n");
    print_vector_f32(fft_output, FFT_N);

    // printf("drawFFT: %dms\n",(int)(HAL_GetTick()-m));
#endif
}

/* Decimate a complex_t buffer into the fft_slice_buff buffer */
void decimateComplexFFTBuffer(complex_t *f_buff, size_t size) {

    uint16_t decimated_block_size = DSP_BLOCK / fft_params.decimation_factor;

    // We decimate in DSP_BLOCK block sizes to save memory, at the expense of speed, since we need two buffers
    // to process the signal (one of DSP_BLOCK length and one of DSP_BLOCK / fft_decimation_factor length)
    float32_t signal[DSP_BLOCK << 1];
    buffer_t<float32_t> src(signal, DSP_BLOCK * 2);
    uint16_t ix = 0;
    uint16_t ixOut = 0;

    while (ix < size) {

        // Transform to float
        dsp::s16_to_f32((adc_type *)f_buff + ix, signal, DSP_BLOCK << 1);

        buffer_t<float> dst((float *)(fft_slice_buff + ixOut), decimated_block_size);
        dst.decimated_size_bytes = decimated_block_size;

        decimator_i.decimate(src, dst, 0, 2);
        decimator_q.decimate(src, dst, 1, 2);

        // Skip the first blocks to account for the delay group of the filter
        if (ix >= FFT_LPF_FIR_FILTER_DELAY_BLOCKS * DSP_BLOCK * fft_params.decimation_factor) {
            ixOut += decimated_block_size;
        }

        ix += DSP_BLOCK;
    }
}

// ADC Acquisition
// OFFLINE. It needs FFN*decimation_factor ADC buffer length
void adquireFFTAsync() {

    /* The FIR filter has a delay of (FFT_LPF_FIR_FILTER_NTAPS-1)/2 samples, so we
     * discard the first ((FFT_LPF_FIR_FILTER_NTAPS-1)/2)/DSP_BLOCK blocks
     * TODO: TO BE IMPLEMENTED. In order to discard blocks, we have to do different from the way it is done
     * in real-time filtering (discarding the first blocks as we are processing them) because
     * here we are filtering AFTER the whole set of blocks is acquired, and so, we have to acquire
     * 'discard_n_blocks' in excess BEFORE we start the filtering and, after that, discard them */
    // uint8_t discard_n_blocks = (uint8_t) (((FFT_LPF_FIR_FILTER_NTAPS - 1) / 2) / DSP_BLOCK) + 1;
    uint16_t fft_buff_size = fft_params.size * fft_params.decimation_factor;

    if (fft_params.decimation_factor > 1) {
        // If we are decimating (using FIR filtering), we have to discard the FIR filter group delay samples
        fft_buff_size += FFT_LPF_FIR_FILTER_DELAY_BLOCKS * DSP_BLOCK * fft_params.decimation_factor;
    }

    union {
        char *c;
        complex_t *f;
    } data;

    uint16_t chunk_size = fft_buff_size * sizeof(complex_t);
    uint64_t timeout = HAL_GetTick() + 1000;

    // Wait for ADC data
    while (fft_fifo.available(&data.c) < chunk_size && HAL_GetTick() < timeout) {
    }

    if (true) { // av >= chunk_size) {

        if (fft_params.decimation_factor > 1) {

            // Decimate the complex buffer (I and Q channels interleaved, so odd and even indexes) into fft_slice_buff
            decimateComplexFFTBuffer((complex_t *)data.c, fft_buff_size);
        } else {
            // Transform to float
            //  dsp::s16_to_f32((adc_type *)data.f, (float32_t *)fft_slice_buff, fft_buff_size << 1);

            // This is weird, i have to invert the components here for the frequency to have the expected direction. It wasn't necessary before and I'm sure
            // I dind't swap them in other places
            for (int i = 0; i < fft_buff_size; i++) {
                fft_slice_buff[i].i = data.f[i].r;
                fft_slice_buff[i].r = data.f[i].i;
            }
        }

        fft_fifo.consume(chunk_size, &data.c);
    }
#if !DSP_FS4_SHIFT
    if (config.fft.removeDC) {
        fft_dcremoval(fft_slice_buffer);
    }
#else
    if (config.fft.removeDC && !dsp::get_freq_shift_enabled()) {
        // In digital mode, the DC is removed in the DSP processor in some cases
        fft_dcremoval(fft_slice_buffer);
    }
#endif
}

complex_t_f32 complexMult(complex_t_f32 a, complex_t_f32 b) {

    complex_t_f32 r;
    r.r = a.r * b.r - a.i * b.i;
    r.i = a.r * b.i + a.i * b.r;

    return r;
}

void fft_work() {

    adquireFFTAsync();

    // Estimate only in the first slice
    // IQ imbalance varies with IF frequency so we are only estimating it in the first slice
    // TODO: Account for IF frequency dependent IQ imbalances

    doFFT();

    if (config.fft.view_mode == FFT_VIEW_TIME_DOMAIN) {

        // drawTimeDomain();

    } else {

        // Depending on the rbw and the bandwidth of interest, we only show a portion of
        // the FFT.
        // In theory, we should be able to use the entire FFT bin array, but
        // the cutoff frequency of the low pass filters in front of the ADCs is lower than
        // the first nyquist zone to avoid aliasing, so the usable portion of the FFT is
        // just the bandwidth of interest, which is lower than half the sampling rate (1st nyquist zone)
        // Reference reading: Analog Devices MT-002: "What the Nyquist Criterion Means to Your Sampled Data System Design by Walt Kester")

        processFFT((float32_t *)fft_slice_buff);

        // A (side) note of caution. When changing connections, be careful not to swap I/Q signals from
        // the quadrature mixer into the ADCs, or the frequency will be inverted again.

        uint32_t peak_ix = getPeak(fft_params.start_bin, fft_params.start_bin + fft_params.nbins, fft_peak_v);

        // Only consider a peak value if it's above a threshold from the current noise floor
        if (fft_peak_v > FFT_SIGNAL_THRESHOLD_DB + fft_noise_floor_db && fft_peak < fft_peak_v) {

            fft_peak = fft_peak_v;
            fft_peak_bin = fft_slice_n * fft_params.nbins + peak_ix - fft_params.start_bin;

            fft_peak_f = fft_params.span_if_start + fft_params.rbw * fft_peak_bin; // config.vfo[config.vfo_ix].freq + (rbw*(peak_ix-(FFT_N>>1)))*1000;
        }
    }
}

/*
 * Performance with -Og optimizations for FFT_N=128: 15ms * number slices + 19ms for the rendering in a 240*80 display buffer at 18Mhz SPI
 */
void updateFFT() {

    uint64_t m;
    uint8_t slices;
    unsigned long first_slice_center_f;

    // Calculate the resolution bandwith (hz per bin) required to have a bin per pixel
    uint64_t last_start_freq = fft_params.span_f_start;
    fft_config(fft_params.span);

    if (last_start_freq != fft_params.span_f_start) {
        // If the span has changed, cancel the smoot factor for a frame so the current values are preserved
        first_frame = true;
    } else {
        first_frame = false;
    }

    if (config.fft.view_mode == FFT_VIEW_TIME_DOMAIN) {
        slices = 1;
    } else {
        slices = fft_params.n_slices;
    }

    first_slice_center_f = fft_params.span_if_start + fft_params.bw;

    fft_peak_v = FFT_MIN_DB;
    fft_peak = FFT_MIN_DB;
    fft_peak_bin = 0;

    m = HAL_GetTick();

    if (config.fft.iq_balance_estimate_period_ms) {
        if (m - last_iqbalance_estimate_ms > config.fft.iq_balance_estimate_period_ms) {
            last_iqbalance_estimate_ms = m;
            fft_estimateIQBalance = true;
        } else {
            fft_estimateIQBalance = false;
        }
    }

    unsigned long f;

    // GPIOA->BSRR= GPIO_PIN_15;
    for (fft_slice_n = 0; fft_slice_n < slices; fft_slice_n++) {

        f = first_slice_center_f +
            ((uint32_t)fft_params.bw << 1U) * (int32_t)fft_slice_n; // move to the next bandwidth of interest (set by the LPF before de ADC)

        //  GPIOB->BSRR= GPIO_PIN_5;
        if (radio::f_iq != f) { // slice change

            radio::f_iq = f;

            // TODO: update_freq() takes 4ms with a 400khz I2C, way too much. Should try to improve it's performance
            // TODO: Changing the frequency of PLLB (Quadrature mixer clock) causes a glich also in PLLA (2nd IF clock) which makes it into the passband
            bool b = if_freq(RF_DIRECTION_RX, f);

            if (!b) {
                status::handleError(status::ST_ERROR, "updateFFT: Error setting IF freq");
            }

            // Clear the FIFO since it will likely contain samples of the previous slice
            fft_fifo.reset();

            // TODO: Check if a delay for fequency settling is needed or not
            HAL_Delay(0);
        }
        //  GPIOB->BSRR= GPIO_PIN_5 << 16;

        fft_work();
    }
    view_manager::mainView.Spectrum()->set_dirty();
}

void fft_loop() {
    updateFFT();
    view_manager::mainView.paint();

    snr_task.run();
    waterfall_task.run();
    iqbalance_task.run();
}
