//
// Enhanced HilbertTransform with configuration support
// Allows setting filter coefficients programmatically
//
#ifndef __DSP_HILBERT_H__
#define __DSP_HILBERT_H__

#include "dsp/dsp_sos.h"

namespace dsp {

/**
 * Enhanced Hilbert Transform with configurable filters
 *
 * Uses frequency-shifting approach with half-band lowpass filters
 * Implements: fs/4 freq shift to create quadrature signals
 */
class HilbertTransform {
  public:
    HilbertTransform();

    /**
     * Configure all three SOSFilters with same coefficients
     * Typically used for half-band lowpass filters
     * @param coeffs Array of 25 coefficients (5 stages * 5 coeffs)
     * @return true if successful
     */
    bool configure(float32_t *coeffs);

    /**
     * Configure filters based on sample rate and bandwidth
     * Auto-generates Butterworth half-band filter coefficients
     * @param sample_rate Sample rate (Hz)
     * @return true if successful
     */
    bool configure(uint32_t sample_rate);

    /**
     * Execute Hilbert transform on one sample
     * @param in Input audio sample
     * @param out_i Output I (real) component
     * @param out_q Output Q (imaginary) component
     */
    void execute(float in, float &out_i, float &out_q);

    /**
     * Reset filter states
     */
    void reset();

  private:
    uint8_t n = 0; // Rotation state (0-3)
    SOSFilter<5> sos_input;
    SOSFilter<5> sos_i;
    SOSFilter<5> sos_q;

    uint32_t sample_rate_ = 0;

    bool configured_ = false;
};

/**
 * Real to Complex converter with magnitude detection
 * Used for specialized applications like APT satellite decoding
 */
class RealToComplex {
  public:
    RealToComplex();

    /**
     * Configure filters
     * @param sample_rate Sample rate (Hz)
     * @return true if successful
     */
    bool configure(uint32_t sample_rate);

    /**
     * Execute conversion and magnitude detection
     * @param in Input real sample
     * @param out_mag_sq_lpf Output: lowpass filtered magnitude squared
     */
    void execute(float in, float &out_mag_sq_lpf);

  private:
    uint8_t n = 0;
    SOSFilter<5> sos_input;
    SOSFilter<5> sos_i;
    SOSFilter<5> sos_q;
    SOSFilter<5> sos_mag_sq;

    uint32_t sample_rate_ = 0;
    bool configured_ = false;
};

} /* namespace dsp */

#endif /*__DSP_HILBERT_H__*/
