//
// Created by Angel Dust on 09/09/2025.
//

#ifndef BEEP_GENERATOR_H
#define BEEP_GENERATOR_H

#include "signal_generator.h"
#include "output.h"
#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"
namespace dsp {
enum BEEP_TYPE {
    BEEP_SINGLE,
    BEEP_DOUBLE,
    BEEP_TRIPLE,
    BEEP_TWO_TONE,
    BEEP_ASCENDING,
    BEEP_SUCCESS,
    BEEP_WARNING,
    BEEP_ERROR,
    BEEP_TYPE_COUNT // This last is for array bounds
};

struct beep_params_st {
    uint32_t frequency1;  // Primary frequency in Hz
    uint32_t frequency2;  // Secondary frequency (for two-tone beeps)
    uint32_t duration_ms; // Duration of each beep in milliseconds
    uint32_t pause_ms;    // Pause between beeps in milliseconds
    uint8_t repetitions;  // Number of beeps
    float gain;           // 0.0 to 1.0
    SIGNAL_SHAPE shape;   // Waveform shape
    uint32_t fade_ms;     // Fade in/out time in milliseconds
};

class BeepGenerator : public Output<complex_t> {
  private:
    SignalGenerator signal_gen;
    beep_params_st params;

    // State management
    struct {
        bool active;
        bool in_beep;
        uint32_t sample_count;
        uint8_t current_rep;
        bool use_second_freq;
    } phase;

    struct {
        uint32_t beep_samples;
        uint32_t pause_samples;
        uint32_t fade_samples;
    } timing;

    // Predefined beep configurations
    static const beep_params_st BEEP_CONFIGS[BEEP_TYPE_COUNT];

    float current_envelope();
    void phase_advance();
    void configure_signal();

  public:
    BeepGenerator();

    void set_sample_rate(uint32_t sample_rate);
    void init(BEEP_TYPE type);
    void init(const beep_params_st &params);
    void restart();
    void stop();

    // Sample generation
    void get_sample(adc_type &sample) override;
    void get_complex_sample(complex_t &sample) override;
    void get_block(buffer_t<complex_t> &buffer) override;

    bool is_beep_active() const {
        return phase.active;
    }

    // Get predefined configuration (useful for customization)
    static beep_params_st get_beep_config(BEEP_TYPE type);
};
} // namespace dsp
#endif // BEEP_GENERATOR_H
