//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_COMMON_H
#define TRX_FRONTEND_DSP_COMMON_H

#include "dsp_config.h"
#include <hw/stm32.h>
#define __FPU_PRESENT 1U
//#define __FPU_USED 1U
#define ARM_MATH_CM4 1

#define DSP_MIN_TX_GAIN_DB -20
#define DSP_MAX_TX_GAIN_DB 20

// FS/4 frequency shift
// Goals:
// - Avoid hardware DC issues (flickr noise, DC leakage)
// - Relax digital DC-blockers requirements, which inevitabily attenuate some band around DC
//
// 1st Attemp (Don't even try. I put it here just to remember why it does not work)
// --------------------------
// After the first decimation, samples are frequency shifted by fs/4
// Then, another decimation by 4 finds the signal of interest at fs, which
// aliases it at DC. This 'trick' to
// Cons:
// This method requires that at least one decimator in the chain finds the signal
// of interest at fs/<decimation factor>
//
// It does not avoid hardware DC leakage or noise, just moves it around (!!!)
//
// Implemented method:
// ------------------
// De-tune by -fs/4 in hardware, then shift +fs/4 in the first decimation/filter phase
//
// Improvement: Do this work also for the FFT so the DC blockers can be removed there?
//
// Note: DC REMOVAL IS ALWAYS REQUIRED for proper demodulatoin
// Improvement: The FS/4 can be done in the decimation loop. This makes the decimator kind of 'impure', but may eventually be necessary
//
// FS/4 limiting requirements:
//
// If we de-tune by FS/4, oversampling is necessary in order to
// - Use the full ADC filter bandwidth
// - Remove the tone that's created after shifting the DC
//
// If we don't decimate, after the hardware shift, the signal of interest will lie at FS/4 from DC. Then, if we center it by re-shifting, some
// band that was filtered before the ADC will be brought into the captured passband.
// If we decimate by at least 4, FS/4 will lie outside the final decimated band, and even after re-shifting to DC, since we have decimated (thus, reducing
// bandwidth) no filtered-out band will appear in the band of interest. However, there are scenarios where we can't affort oversampling while capturing a wide
// bandwidth

#define DSP_FS4_SHIFT 1

// Manual PKHBT: pack halfword bottom
#if !defined(__PKHBT)
#define __PKHBT(a, b) (((b)&0xFFFF) | ((a & 0xFFFF) << 16))
#endif
// Manual PKHTB: pack halfword top
#if !defined(__PKHTB)
#define __PKHTB(a, b) (((a)&0xFFFF0000) | (((b) >> 16) & 0xFFFF))
#endif

#include <stddef.h>
#include <stdint.h>
#include <arm_math.h>
#include <Signal.h>
#include "FIFO.h"

typedef int16_t adc_type;
typedef uint32_t adc_type_complex_union;

union complex_t {
    struct {
        adc_type r;
        adc_type i;
    };
    adc_type_complex_union _rep;
};

typedef struct {
    float32_t i;
    float32_t r;
} complex_t_f32;

#define SWAP_PTR(a, b)                                                                                                                                         \
    do {                                                                                                                                                       \
        void *_tmp = (a);                                                                                                                                      \
        (a) = (b);                                                                                                                                             \
        (b) = (decltype(a))_tmp;                                                                                                                               \
    } while (0)

enum DSP_COMMAND { DSP_COMMAND_NONE, DSP_COMMAND_STOP, DSP_COMMAND_START };
enum DSP_STATUS { DSP_STATUS_STOPPED, DSP_STATUS_STOPPING, DSP_STATUS_RUNNING, DSP_STATUS_PENDING };
enum DSP_ERROR {
    DSP_ERR_NONE,
    DSP_ERR,
    DSP_ERR_FILEOPEN,
    DSP_ERR_FILECLOSE,
    DSP_ERR_FILEWRITE,
    DSP_ERR_FILEREAD,
    DSP_ERR_DMAOVERRUN,
    DSP_ERR_FIFO_OVERRUN,
    DSP_ERR_FIFO_UNDERRUN
};

// Max bandwidth of the DSP. This is the cutoff frequency of the low pass filters before the ADCs.
// Note the complex bandwidth is twice since we're sampling quadrature signals
#define DSP_BANDWIDTH 250000
// The usable percentage of the DSP bandwidth. We discard frequencies on the
// transition band of the low pass filter
#define USABLE_BW_FACTOR 0.80
#define MAX_DECIMATION_FACTOR 8

#define DSP_MAX_CAPTURE_SIZE 50000000
#define FIR_DECIMATOR_1ST_HALFBAND_TAPS 23
#define FIR_DECIMATOR_SIGNAL_TAPS 37

// IF LCD and SD CARD share the same SPI bus, we need to disable the LCD when capturing o replaying to prevent the ADC DMA to interrupt
// A LCD SPI DMA transfer and cause problems
#ifndef STM32F4xx
#define LCD_DISABLE_ON_DSP false
#endif

// Other way to try to use the same SPI bus is by executing DSP tasks (which should be less time critical than DSP processors) in the
// main loop rather than within the timing interrupt. This may not work if we have too much load in our loop and we don't give enough
// chances to the task to execute at decent pace
#define EXECUTE_TASKS_ON_INTERRUPT 1

/*
 * Direction of the baseband flow
 */
enum DSP_DIRECTION {
    DSP_DIRECTION_IN = 0,
    DSP_DIRECTION_OUT,
    DSP_DIRECTION_INOUT // BOTH
};

struct st_dsp_status {

    uint8_t id;
    volatile DSP_STATUS status = DSP_STATUS_STOPPED;
    volatile DSP_ERROR error = DSP_ERR_NONE;
    DSP_DIRECTION direction = DSP_DIRECTION_IN;

    volatile float gain{1.0}; // This is the gain factor. Not in DB

    uint32_t bandwidth;
    uint32_t sample_rate;
    uint8_t decimation_factor;
    uint32_t decimated_block_size_bytes;
    uint16_t decimated_block_size;
    uint16_t bits_per_sample;
    uint8_t n_channels;

    volatile uint32_t block_size_bytes; // Size of each processed block, in bytes
    volatile uint64_t processed_blocks;
    volatile uint32_t fifo_underruns;
    volatile uint32_t fifo_overruns;

    uint64_t start_ms;
    uint64_t stop_ms;
    uint64_t last_error_ms;

    bool operator==(const st_dsp_status &st) const {
        return status == st.status && error == st.error && fifo_underruns == st.fifo_underruns && fifo_overruns == st.fifo_overruns &&
               sample_rate == st.sample_rate && processed_blocks == st.processed_blocks && block_size_bytes == st.block_size_bytes &&
               bits_per_sample == st.bits_per_sample && n_channels == st.n_channels && id == st.id && gain == st.gain &&
               decimation_factor == st.decimation_factor && bandwidth == st.bandwidth && last_error_ms == st.last_error_ms;
        ;
    }
    uint32_t elapsed_ms() {
        return ((stop_ms ? stop_ms : HAL_GetTick()) - start_ms);
    }
    float drop_rate() {
        return processed_blocks ? (((float)(fifo_overruns) / (float)processed_blocks)) : 0;
    }
    float drop_freq() {
        volatile uint32_t elapsed = elapsed_ms();
        return ((float)(fifo_overruns)) / ((float)elapsed / 1000.0f);
    }
    float starve_rate() {
        return processed_blocks ? (((float)(fifo_underruns) / (float)processed_blocks)) : 0;
    }
    float starve_freq() {
        volatile uint32_t elapsed = elapsed_ms();
        return ((float)(fifo_underruns)) / ((float)elapsed / 1000.0f);
    }
    void reset() {
        start_ms = HAL_GetTick();
        processed_blocks = 0;
        stop_ms = 0;
        fifo_overruns = 0;
        fifo_underruns = 0;
    }
};

namespace dsp {

struct st_test_signal_params {
    int8_t pulse_duty = 50;
    uint32_t baseband_frequency = 1000;
    uint32_t modulation_frequency = 1000;
};

struct st_dsp_config {
    int8_t gain = DSP_MIN_TX_GAIN_DB;
    bool audio_compressor_enabled = true;
    int32_t audio_compressor_threshold = -30;
    st_test_signal_params test_signal;
};

struct st_timestamp {
    uint32_t date{0};
    uint32_t time{0};
};

extern st_dsp_config dsp_config;

extern Signal dsp_common_params_signal;

extern st_dsp_status *dsp_status;

// Current maximum sample frequency. It depends on whether we're doing more or less real time processing to the ADC buffer
extern uint32_t dsp_max_sample_rate;
extern const char *dsp_error_names[];

/* Sets the max sample frequency depending on whether we're doing real-time DSP or not */
void set_max_sample_freq(bool dsp);

/* Set a specific maximum for the sample rate */
void set_max_sample_freq(uint32_t rate);

int32_t get_frequency_shift(uint32_t sample_rate = 0);

/* Sets the digital domain TX direction gain */
void set_tx_gain_db(int8_t gain_db);

void s16_to_q15(const adc_type *src, q15_t *dst, size_t size);
void s16_to_f32(const adc_type *src, float32_t *dst, size_t size);

void q15_to_s16(const q15_t *src, adc_type *dst, size_t size);
void f32_to_s16(const float32_t *src, adc_type *dst, size_t size);

void unzip_c16(const adc_type *__restrict src, adc_type *__restrict dst_i, adc_type *__restrict dst_q, size_t n_samples);
void zip_c16(const adc_type *__restrict src_i, adc_type *__restrict src_q, adc_type *__restrict dst, size_t n_samples);
void unzip_f32(const float32_t *src, float32_t *dst_i, float32_t *dst_q, size_t n_samples);
void zip_f32(const float32_t *src_i, float32_t *src_q, float32_t *dst, size_t n_samples);

void rotate_fs4_q15(const q15_t *src, q15_t *dst, size_t n_samples);
// void rotate_fs8_q15(const q15_t *src, q15_t *dst, size_t n_samples);
void rotate_fs4_f32(const float32_t *src, float32_t *dst, size_t n_samples);

void set_config(st_dsp_config &);
st_dsp_config get_config();

} // namespace dsp

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif // TRX_FRONTEND_DSP_COMMON_H
