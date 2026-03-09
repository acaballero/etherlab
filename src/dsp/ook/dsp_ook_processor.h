//
// OOK (On-Off Keying) Transmit Processor
//

#ifndef TRX_FRONTEND_DSP_OOK_PROCESSOR_H
#define TRX_FRONTEND_DSP_OOK_PROCESSOR_H

#include "dsp/dsp_common.h"
#include "dsp/dsp_processor.h"
#include "dsp/blocks/signal_generator.h"
#include "types.h"
#include <cstdint>
#include <vector>

class DspOOKProcessor : public DspProcessor {

  public:
    DspOOKProcessor() {
        this->info.direction = DSP_DIRECTION_OUT;
    }

    void work(const buffer_t<adc_type> *buffer) override;

    bool wait_first_block() override {
        return false;
    }

    /**
     * Configure the OOK processor timing.
     * Mark and space have independent durations. Each bit in the sequence
     * produces either a carrier-on interval (mark) or a carrier-off interval (space).
     *
     * @param carrier_freq      Carrier frequency in Hz (relative to baseband, 0 = at DC)
     * @param mark_duration_us  Duration of a "1" bit in microseconds
     * @param space_duration_us Duration of a "0" bit in microseconds
     * @param pause_us          Silence gap between repetitions in microseconds
     * @param sample_rate       DAC sample rate in Hz
     */
    void set_config(uint32_t carrier_freq, uint32_t mark_duration_us, uint32_t space_duration_us,
                    uint32_t pause_us, uint32_t sample_rate);

    /** Set the OOK bit sequence. Each element is 0 (off) or 1 (on). */
    void set_sequence(const std::vector<uint8_t> &seq);

    void set_loop(bool v) {
        loop = v;
    }
    bool get_loop() const {
        return loop;
    }

    void set_repetitions(uint16_t reps) {
        repetitions = reps;
    }
    uint16_t get_repetitions() const {
        return repetitions;
    }

    bool is_finished() const {
        return finished;
    }

    size_t get_current_bit_index() const {
        return bit_index;
    }
    uint16_t get_current_rep() const {
        return current_rep;
    }
    size_t get_sequence_length() const {
        return sequence.size();
    }

    uint32_t get_mark_duration_us() const {
        return mark_duration_us;
    }
    uint32_t get_space_duration_us() const {
        return space_duration_us;
    }
    uint32_t get_pause_us() const {
        return pause_us;
    }

    /**
     * Compute the total duration of one repetition of the sequence in microseconds,
     * accounting for independent mark/space durations.
     */
    uint32_t get_frame_duration_us() const;

  protected:
    dsp::SignalGenerator carrier;

    std::vector<uint8_t> sequence;

    uint32_t mark_duration_us{500};
    uint32_t space_duration_us{500};
    uint32_t pause_us{5000};
    uint32_t cached_sample_rate{0};

    // Sample counts derived from timing
    float32_t mark_samples{0};
    float32_t space_samples{0};
    float32_t pause_samples{0};

    float32_t sample_accumulator{0};
    float32_t current_interval_len{0}; // Length of current interval (mark, space, or pause) in samples

    size_t bit_index{0};
    uint16_t repetitions{1};
    uint16_t current_rep{0};
    bool loop{false};
    bool finished{false};
    bool in_pause{false}; // True when outputting silence between repetitions
};

#endif // TRX_FRONTEND_DSP_OOK_PROCESSOR_H
