//
// Created by Angel Dust on 04/06/2025.
//

#ifndef __AFSK_TX_TASK_H__
#define __AFSK_TX_TASK_H__

#include "dsp/aprs/aprs_packet.h"
#include "dsp/modulation/dsp_demodulate.h"
#include "dsp/receive/receive_task_base.h"
#include "dsp/task.h"
#include <stdint.h>

namespace dsp {

#define AFSK_SAMPLERATE 1536000
#define AFSK_DELTA_COEF ((1ULL << 32) / AFSK_SAMPLERATE)
#define AFSK_MAX_PACKET_SIZE 256

class AFSKTXTask : public Task {
  public:
    void configure(uint32_t phase_inc_mark, uint32_t phase_inc_space, uint8_t repeat, uint8_t symbol_count, uint32_t bandwidth, uint16_t *packet_ptr);

    void work() override;

    bool start() override;

    void stop() override;

  private:
    uint32_t afsk_samples_per_bit{0};
    uint32_t afsk_phase_inc_mark{0};
    uint32_t afsk_phase_inc_space{0};
    uint8_t afsk_repeat{0};
    uint32_t fm_delta{0};
    uint8_t symbol_count{0};

    uint8_t repeat_counter{0};
    uint8_t bit_pos{0};
    uint16_t *word_ptr{};
    std::string packet{};
    uint16_t cur_word{0};
    uint8_t cur_bit{0};
    uint32_t sample_count{0};
    uint32_t tone_phase{0}, phase{0}, sphase{0};
    int32_t tone_sample{0}, delta{0};

    int8_t re{0}, im{0};
};
} // namespace dsp

#endif /*__AFSK_TX_TASK__*/
