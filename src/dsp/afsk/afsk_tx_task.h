//
// Created by Angel Dust on 04/06/2025.
//

#ifndef __AFSK_TX_TASK_H__
#define __AFSK_TX_TASK_H__

#include "dsp/task.h"
#include <stdint.h>
#include <vector>

namespace dsp {

#define AFSK_MAX_PACKET_SIZE 256
#define AFSK_MAX_DELAY_MS 300
#define AFSK_MAX_TAIL_MS 100

class AFSKTXTask : public Task {
  public:
    using Task::Task;
    void configure(uint32_t phase_inc_mark, uint32_t phase_inc_space, uint8_t repeat, uint8_t symbol_count, uint32_t bandwidth = 10000, uint16_t delay_ms = 0,
                   uint16_t tail_ms = 0);

    void set_data(uint16_t *data);

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
    uint16_t delay_tail_ms{0};
    uint16_t delay_front_ms{0};
    uint32_t fill_bits{0};
    bool tail_sent{false};
    uint32_t afsk_delta_coeff{0};
    uint8_t repeat_counter{0};
    uint8_t bit_pos{0};
    uint16_t packet_ix{0};
    std::vector<uint16_t> packet;

    uint8_t cur_bit{0};
    uint32_t sample_count{0};
    uint32_t tone_phase{0}, phase{0}, sphase{0};
    int32_t tone_sample{0}, tone_sample_last{0}, delta{0};

    int8_t re{0}, im{0};
};
} // namespace dsp

#endif /*__AFSK_TX_TASK__*/
