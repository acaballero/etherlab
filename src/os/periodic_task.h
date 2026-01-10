//
// Created by Angel Dust on 29/06/2022.
//

#ifndef TRX_FRONTEND_PERIODIC_TASK_H
#define TRX_FRONTEND_PERIODIC_TASK_H

#include "stdio.h"
#include "stdint.h"
#include <cstring>
#include <functional>
#include <stm32f4xx.h>

namespace os {

typedef std::function<void()> callback_t;

class periodic_task {
  public:
    periodic_task(uint64_t period_ms, callback_t f, uint64_t duration_ms = 0, uint32_t delay_ms = 0, const char *name = nullptr)
        : _period_ms(period_ms), _duration_ms(duration_ms), _callback(f) {
        if (_duration_ms) {
            _end_ms = HAL_GetTick() + _duration_ms;
        }

        if (delay_ms) {
            _next_ms = HAL_GetTick() + delay_ms;
        }

        if (name) {
            set_name(name);
        }
    };
    void set_period(uint64_t period) {
        _period_ms = period;
        _next_ms = HAL_GetTick() + _period_ms;
    }
    uint64_t get_period() {
        return _period_ms;
    }
    void set_enabled(bool b);

    bool get_enabled();

    void set_name(const char *str) {
        strncpy(name, str, 4);
    }

    void set_id(int id) {
        this->id = id;
    };

    int get_id() {
        return id;
    };

    // Set next execution time
    void set_next(uint64_t ms);

    uint64_t get_duration();
    uint64_t get_last_time();
    uint64_t get_end_time();
    uint64_t get_start_time();
    char *get_name() {
        return name[0] ? name : nullptr;
    }

    void set_high_priority(bool b) {
        _is_high_priority = b;
    }
    bool is_high_priority() const {
        return _is_high_priority;
    }

    bool ready_to_run(uint64_t current_time) const {
        return _period_ms && enabled && current_time >= _next_ms;
    }
    char *get_log(char *buf);

    bool finished();

    void run();

  private:
    int id{0};
    // Period
    uint64_t _period_ms{0};
    // First execution time
    uint64_t _start_ms{0};
    // End time
    uint64_t _end_ms{0};
    // Last execution time
    uint64_t _last_ms{0};
    // Next execution time
    uint64_t _next_ms{0};
    // Duration of the task in milliseconds
    uint64_t _duration_ms{0};

    // Current rate of execution
    float _rate{0};

    bool enabled{true};

    // Callback function implementing the task work
    callback_t _callback;

    bool _is_high_priority{0};

    char name[5] = "";
};
} // namespace os
#endif // TRX_FRONTEND_PERIODIC_TASK_H
