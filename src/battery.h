//
// Created by Angel Dust on 16/01/2022.
//

#ifndef TRX_FRONTEND_BATTERY_H
#define TRX_FRONTEND_BATTERY_H

#include <stdio.h>

#include "Signal.h"
#include "os/periodic_task.h"
#include <hw/stm32.h>

// A voltage divider is used to bring the input voltage to an appropriate level
// for the ADC's input range
#define VOLTAGE_DIVISION_RATIO 6.05
#define INPUT_VOLTAGE_MAX 12.0
#define BATTERY_VOLTAGE_80 11.2         // 3S pack at 80% charge: 3.7 V * 3
#define BATTERY_VOLTAGE_20 10.8         // 3S pack at 20% charge: 3.6 V * 3
#define BATTERY_STATUS_HYSTERESIS 0.007 // Amount of hysteresis applied to status changes

namespace battery {

enum BATTERY_STATUS {
    BATTERY_STATUS_UNDEFINED,
    BATTERY_STATUS_VERY_LOW,
    BATTERY_STATUS_LOW,
    BATTERY_STATUS_MEDIUM,
    BATTERY_STATUS_HIGH,
    BATTERY_STATUS_CHARGING
};

struct battery_st_info {

    float voltage = 0;
    uint8_t capacity = 0;
    BATTERY_STATUS status = BATTERY_STATUS_UNDEFINED;

    bool operator==(const battery_st_info &st) const { return voltage == st.voltage && capacity == st.capacity; }
};

extern battery_st_info battery_info;
extern Signal battery_signal;
extern os::periodic_task task;

} // namespace battery

#endif // TRX_FRONTEND_BATTERY_H
