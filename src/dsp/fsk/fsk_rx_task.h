

#ifndef __FSK_RX_TASK_H__
#define __FSK_RX_TASK_H__

#include "dsp/buffer.hpp"
#include "dsp/dsp_common.h"
#include "dsp/audio/fm_squelch.h"
#include "dsp/receive/receive_task_base.h"
#include <cstdint>
#include <functional>

class FSKRXTask : public ReceiveTaskBase {
  public:
    using ReceiveTaskBase::ReceiveTaskBase;

  private:
    size_t baseband_fs = 1024000; // aka: sample_rate

    void configure(size_t deviation, uint32_t sample_rate);
    void flush();
    void send_packet(uint32_t data);
    void process_bits(const buffer_t<uint8_t> &buffer);

    void clear_data_bits();
    void handle_sync(bool inverted);

    /* Returns true if the batch has as sync frame. */
    bool has_sync() const {
        return has_sync_;
    }

    /* Set once app is ready to receive messages. */
    bool configured = false;

    size_t deviation = 3750;

    size_t channel_decimation = 2;
    int32_t channel_filter_low_f = 0;
    int32_t channel_filter_high_f = 0;
    int32_t channel_filter_transition = 0;

    /* Squelch to ignore noise. */
    FMSquelch squelch{};
    uint64_t squelch_history = 0;

    /* Used to keep track of how many samples were processed
     * between status update messages. */
    uint32_t samples_processed = 0;

    /* Number of bits in 'data_' member. */
    static constexpr uint8_t data_bit_count = sizeof(uint32_t) * 8;

    /* Sync frame codeword. */
    static constexpr uint32_t sync_codeword = 0x12345678;

    /* When true, sync frame has been received. */
    bool has_sync_ = false;

    /* When true, bit vales are flipped in the codewords. */
    bool inverted = false;

    uint32_t data = 0;
    uint8_t bit_count = 0;
    uint8_t word_count = 0;

    /* LPF to reduce noise. POCSAG supports 2400 baud, but that falls
     * nicely into the transition band of this 1800Hz filter.
     * scipy.signal.butter(2, 1800, "lowpass", fs=24000, analog=False) */
    DspIIRDecimator<2> lowpass_filter;

    void process_audio(buffer_t<float32_t> &buffer) override;
    bool init() override;
};

#endif
