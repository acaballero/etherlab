//
// Improved modulators for STM32F427 - Final Version
// Uses buffer_t<complex_t_f32> with overloaded work() methods
// Automatic filter coefficient calculation like decimators
//
#ifndef __DSP_MODULATE_H__
#define __DSP_MODULATE_H__

#include "dsp/dsp_common.h"
#include "dsp/buffer.hpp"
#include "dsp_hilbert.h"
#include "arm_math.h"

namespace dsp {

/**
 * Base modulator class
 * Input: Real audio signal (mono, float32)
 * Output: Complex IQ baseband signal
 */
class modulator {
  public:
    virtual ~modulator() = default;

    /**
     * Main work method - buffer_t interface
     * @param audio_in Input audio buffer (mono, normalized ±1.0)
     * @param iq_out Output IQ buffer (complex_t_f32 with .i and .r members)
     */
    virtual void work(const float32_t *audio_in, buffer_t<complex_t_f32> &iq_out) = 0;

    /**
     * Overloaded work method - separate I/Q arrays
     * @param audio_in Input audio samples
     * @param iq_out_i Output I channel
     * @param iq_out_q Output Q channel
     * @param count Number of samples
     */
    virtual void work(const float32_t *audio_in, float32_t *iq_out_i, float32_t *iq_out_q, size_t count) = 0;
};

/**
 * AM modulator
 * output = (carrier + modulation_index * audio) on both I and Q
 */
class am_modulator : public modulator {
  public:
    void work(const float32_t *audio_in, buffer_t<complex_t_f32> &iq_out) override;
    void work(const float32_t *audio_in, float32_t *iq_out_i, float32_t *iq_out_q, size_t count) override;

    void set_modulation_index(float32_t m) {
        mod_index = m;
    }
    float32_t get_modulation_index() const {
        return mod_index;
    }

  private:
    float32_t mod_index = 0.5f;
};

/**
 * DSB modulator (Double Sideband - suppressed carrier)
 * output = audio on both I and Q (no carrier)
 */
class dsb_modulator : public modulator {
  public:
    void work(const float32_t *audio_in, buffer_t<complex_t_f32> &iq_out) override;
    void work(const float32_t *audio_in, float32_t *iq_out_i, float32_t *iq_out_q, size_t count) override;
};

/**
 * SSB modulator (USB/LSB selectable)
 * Uses Hilbert transform to generate quadrature signal
 * Filters are automatically configured based on sample rate and bandwidth
 */
class ssb_modulator : public modulator {
  public:
    enum Mode {
        USB, // Upper sideband
        LSB  // Lower sideband
    };

    ssb_modulator(Mode mode = USB) : mode(mode) {
    }

    void work(const float32_t *audio_in, buffer_t<complex_t_f32> &iq_out) override;
    void work(const float32_t *audio_in, float32_t *iq_out_i, float32_t *iq_out_q, size_t count) override;

    /**
     * Configure SSB modulator
     * Automatically calculates Hilbert filter coefficients
     * @param sample_rate Audio sample rate (Hz)
     * @param bandwidth SSB bandwidth (Hz) - typically 2400-3000 Hz
     * @return true if successful
     */
    bool configure(uint32_t sample_rate, uint32_t bandwidth);

    void set_mode(Mode mode) {
        mode = mode;
    }
    Mode get_mode() const {
        return mode;
    }

  private:
    Mode mode;
    HilbertTransform hilbert;
    uint32_t sample_rate = 0;
    uint32_t bandwidth = 0;
    bool configured = false;
};

/**
 * FM modulator (Narrowband and Wideband)
 * Phase modulation with proper accumulation
 */
class fm_modulator : public modulator {
  public:
    void work(const float32_t *audio_in, buffer_t<complex_t_f32> &iq_out) override;
    void work(const float32_t *audio_in, float32_t *iq_out_i, float32_t *iq_out_q, size_t count) override;

    /**
     * Configure FM parameters
     * @param sample_rate Audio sample rate (Hz)
     * @param deviation Maximum frequency deviation (Hz)
     *                  NFM: ~2.5-5 kHz, WFM: ~75 kHz
     */
    void configure(float32_t sample_rate, float32_t deviation);

    float32_t get_deviation() const {
        return deviation;
    }
    float32_t get_sample_rate() const {
        return sample_rate;
    }

  private:
    float32_t kf = 0.0f;    // Modulation constant
    float32_t phase = 0.0f; // Phase accumulator
    float32_t sample_rate = 0.0f;
    float32_t deviation = 0.0f;

    // Fast sine/cosine using CMSIS
    inline void get_sin_cos(float32_t phase, float32_t &sin_val, float32_t &cos_val);
};

} /* namespace dsp */

#endif /*__DSP_MODULATE_H__*/
