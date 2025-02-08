//
// Created by Angel Dust on 16/10/2019.
//

#ifndef TRX_FRONTEND_FFT_H
#define TRX_FRONTEND_FFT_H

#define CALIBRATE_FFT 1
#define DEBUG_FFT 0
#define DEBUG_FFT_ADC 0

#define __FPU_PRESENT 1U
#define __FPU_USED 1U
#define ARM_MATH_CM4 1

#include <stdio.h>
#include <stddef.h>
#include <arm_math.h>
#include "hw/stm32.h"
#include "config.h"
#include "../../../lib/ST77XX-STM32/Display_afb.h"
#include "dsp/dsp_config.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_buffers.h"
#include "FFTIQBalancer.h"
#include "FIFOv1.h"
#include "os/periodic_task.h"

typedef float32_t fft_type;

#define FFT_SCALE_FACTOR 0
#define FTT_DISPLAY_WIDTH DISPLAY_X_PIXELS

extern arm_cfft_instance_f32 S_cfft;

extern void (*arm_cfft)(const arm_cfft_instance_f32 *, float32_t *, uint8_t, uint8_t);

extern void (*arm_cmplx_mag)(float32_t *pSrc, float32_t *pDst, uint32_t numSamples);

enum FFT_STATUS { FFT_STATUS_READY, FFT_STATUS_ADQUIRING, FFT_STATUS_IDLE, FFT_STATUS_FAULT };

typedef struct st_fft_params {

    uint32_t span;

    uint16_t size;

    // Resolution bandwidth per bin
    float rbw;

    // Number of usable bins of each FFT
    uint16_t nbins;

    uint16_t total_bins;

    // Usable bandwidth (half the bandwidth, actually) of each slice
    uint32_t bw = 0;

    uint8_t n_slices = 1;

    uint8_t decimation_factor = 0; // 0: not initialized

    uint32_t sample_freq = 0;

    float bin_width_px = MAXFLOAT;

    // Slice width, in screen pixels
    uint16_t slice_w_px = 0;

    // Start bin of each FFT
    uint8_t start_bin = 0;

    // Resolution bandwidth at the display
    float display_rbw = 0;

    // Absolute start frequency of the span
    uint64_t span_f_start;

    // Starting intermediate frequency of the span
    uint64_t span_if_start;

    void calc();

    bool valid();

} st_fft_params;

namespace fft {
extern os::periodic_task fft_task;
extern os::periodic_task iqbalance_task;
extern os::periodic_task waterfall_task;
} // namespace fft

complex_t_f32 complexMult(complex_t_f32 a, complex_t_f32 b);
void processFFT(float32_t *v);
bool fft_config(uint32_t span);
void fftInit();
void resetIQBalancer();
uint8_t getPeak(uint8_t start_bin, uint8_t end_bin, fft_type &peak_v);
void adquireFFTAsync();
void reorderBins(complex_t_f32 *v);
void calcFFTRange();
uint8_t findFreqs(int *arr_idx_freqs, uint8_t max);

extern st_fft_params fft_params;
extern FFTIQBalancer fftIQBalancer;
extern fft_type fft_peak;
extern uint64_t fft_peak_f;
extern uint8_t fft_slice_n;
extern uint16_t fft_peak_bin;
extern bool fft_min_db_auto;
extern bool fft_estimateIQBalance;
extern uint16_t fft_calc_noise_floor_period_ms;
extern float fft_noise_floor_db; // Noise floor in dB
// extern float fft_range;
extern bool fft_mag_overload;
extern volatile FFT_STATUS fft_status;
extern fft_type fft_peak_v;

extern FIFO fft_fifo;
extern fft_type fft_display[FTT_DISPLAY_WIDTH];
extern fft_type fft_display_db[FTT_DISPLAY_WIDTH];
extern complex_t_f32 fft_slice_buff[FFT_N];
extern buffer_t<float32_t> fft_slice_buffer;
extern fft_type fft_output[FFT_N];
extern float window[FFT_N];

#endif // TRX_FRONTEND_FFT_H
