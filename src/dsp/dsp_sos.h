//
// Complete SOSFilter implementation
// Second-Order Section (SOS) cascaded biquad filter using CMSIS-DSP
//
#ifndef __DSP_SOS_H__
#define __DSP_SOS_H__

#include "dsp_common.h"
#include "stdio.h"
#include "stdint.h"

/**
 * Second-Order Section (SOS) filter
 * Implements a cascade of biquad (second-order) IIR filters
 *
 * Uses CMSIS-DSP arm_biquad_cascade_df1_f32 for efficient execution
 *
 * Template parameter N: Number of biquad stages (cascaded sections)
 */
template <size_t N> class SOSFilter {
  public:
    SOSFilter() {

        // Initialize the CMSIS structure
        biquad_instance.numStages = N;
        biquad_instance.pState = state_;

        reset();
    }

    /**
     * Configure filter with biquad coefficients
     * @param coeffs Array of coefficients in format: [b0, b1, b2, a1, a2, b0, b1, b2, a1, a2, ...]
     *               Note: a0 is assumed to be 1.0, coefficients should be normalized
     *               a1, a2 are NEGATED as per CMSIS-DSP convention
     */
    void configure(float32_t coeffs[5 * N]) {

        biquad_instance.pCoeffs = coeffs;
        // Re-initialize with new coefficients
        arm_biquad_cascade_df1_init_f32(&biquad_instance, N, coeffs, state_);
    }

    /**
     * Execute filter on a single sample
     * @param value Input sample
     * @return Filtered output sample
     */
    inline float32_t execute(float32_t value) {
        float32_t output;
        arm_biquad_cascade_df1_f32(&biquad_instance, &value, &output, 1);
        return output;
    }

    /**
     * Execute filter on a block of samples
     * @param input Input buffer
     * @param output Output buffer
     * @param block_size Number of samples to process
     */
    void execute_block(float32_t *input, float32_t *output, uint32_t block_size) {
        arm_biquad_cascade_df1_f32(&biquad_instance, input, output, block_size);
    }

    /**
     * Reset filter state (clear delay line)
     */
    void reset() {
        for (size_t i = 0; i < 4 * N; i++) {
            state_[i] = 0.0f;
        }
    }

    /**
     * Get number of stages
     */
    constexpr size_t num_stages() const {
        return N;
    }

  private:
    arm_biquad_casd_df1_inst_f32 biquad_instance;

    float32_t state_[4 * N]; // State variables: 4 per biquad stage
};

#endif /*__DSP_SOS_H__*/
