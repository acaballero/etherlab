//
// Created by Angel Dust on 16/10/2019.
//

#ifndef TRX_FRONTEND_FFT_H
#define TRX_FRONTEND_FFT_H

#include <utility>
#define CALIBRATE_FFT 1
#define DEBUG_FFT 0
#define DEBUG_FFT_ADC 0

#define __FPU_PRESENT 1U
#define __FPU_USED 1U
#define ARM_MATH_CM4 1

#include <stdio.h>
#include <stddef.h>
#include "hw/stm32.h"
#include "config.h"
#include "../../../lib/ST77XX-STM32/Display_afb.h"
#include "dsp/dsp_config.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_buffers.h"
#include "FFTIQBalancer.h"
#include "FIFOv1.h"
#include "os/periodic_task.h"
#include "fft_params.h"
typedef float32_t fft_type;

#define FFT_SCALE_FACTOR 0
#define FTT_DISPLAY_WIDTH DISPLAY_X_PIXELS

extern arm_cfft_instance_f32 S_cfft;

extern void (*arm_cfft)(const arm_cfft_instance_f32 *, float32_t *, uint8_t, uint8_t);

extern void (*arm_cmplx_mag)(float32_t *pSrc, float32_t *pDst, uint32_t numSamples);

enum FFT_STATUS { FFT_STATUS_READY, FFT_STATUS_ADQUIRING, FFT_STATUS_IDLE, FFT_STATUS_FAULT };

namespace fft {
void set_max_slices(uint8_t);
extern uint8_t current_max_slices;
extern os::periodic_task fft_task;
extern os::periodic_task iqbalance_task;
extern os::periodic_task waterfall_task;
extern float fft_noise_floor_db; // Noise floor in dB
extern float snr;                // Signal to noise in the baseband
extern float dbm;                // Power in the baseband
extern float dbm_instant;
extern float dbm_peak;
extern adc_type adc_max_ampl;
std::pair<int, int> get_bandwidth_pixel_range();
void set_waterfall_speed(uint16_t);
void apply_fft_params(st_fft_params);
extern Signal signal;
} // namespace fft

complex_t_f32 complex_mult(complex_t_f32 a, complex_t_f32 b);
void process_fft(float32_t *v);
bool fft_config(uint32_t span);
void fft_init();
void reset_iq_balancer();
uint32_t get_peak(uint32_t start_bin, uint32_t end_bin, fft_type &peak_v);
void adquire_fft_async();
void reorder_bins(complex_t_f32 *v);
void calc_fft_range();

extern FFTIQBalancer fftIQBalancer;
extern fft_type fft_peak;
extern uint64_t fft_peak_f;
extern uint8_t fft_slice_n;
extern uint16_t fft_peak_bin;
extern bool fft_min_db_auto;
extern bool fft_estimateIQBalance;
extern uint16_t fft_calc_noise_floor_period_ms;

extern volatile FFT_STATUS fft_status;
extern fft_type fft_peak_v;

extern FIFO fft_fifo;
extern fft_type fft_display[FTT_DISPLAY_WIDTH];
extern fft_type fft_display_db[FTT_DISPLAY_WIDTH];
extern complex_t_f32 fft_slice_buff[FFT_N];
extern fft_type fft_output[FFT_N];
extern float window[FFT_N];

#endif // TRX_FRONTEND_FFT_H
