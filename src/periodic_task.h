//
// Created by Angel Dust on 29/06/2022.
//

#ifndef TRX_FRONTEND_PERIODIC_TASK_H
#define TRX_FRONTEND_PERIODIC_TASK_H

#include "stdio.h"

class periodic_task {
public:
    periodic_task(uint64_t period_ms, void(*f)(void)) : _period_ms(period_ms), _callback(f) {};
    void set_period(uint64_t period) { _period_ms = period; }
    void loop();
private:
    uint64_t _period_ms;
    uint64_t _last_ms;
    void(*_callback)(void);
};

#endif //TRX_FRONTEND_PERIODIC_TASK_H
