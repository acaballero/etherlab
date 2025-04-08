//
// Created by Angel Dust on 16/01/2022.
//

#include "battery.h"
#include "hw/stm32.h"
#include "config.h"
#include "os/periodic_task.h"
#include "Signal.h"

namespace battery {

void check_battery();

battery_st_info battery_info;
os::periodic_task task(2000, check_battery);
Signal battery_signal;

void update_battery_info() {

    uint16_t adcv = GetADCValue(&BATTERY_VOLTAGE_ADC_HANDLER, INPUT_VOLTAGE_ADC_CHANNEL, 2);
    float v = ((float)adcv / (float)MAX_ADC_VALUE) * V_REF * VOLTAGE_DIVISION_RATIO;

    // Round to 2 decimal places (well, kind off since that's literrally impossible with float -- and even double --)
    v = roundf(v * 100.0f) / 100.0f;

    float delta = battery_info.voltage - v;

    if (abs(delta) / fmin(battery_info.voltage, v) < 25) {
        // exponential filter
        float filter_factor = 0.4;
        battery_info.voltage = (battery_info.voltage - (filter_factor * delta));
    } else {
        // If delta>25% (which may occur at startup or when the charger is plugged-in) we won't smooth it
        battery_info.voltage = v;
    }

    // Round to just 1 decimal place
    battery_info.voltage = roundf(battery_info.voltage * 10.0f) / 10.0f;

    // Without measuring the outgoing current is difficult to calculate the exact remaining charge, so we are just checking whether it's above 80%
    // or below 20% and setting the capacity as 100%, 50% or 10%
    battery_info.capacity = battery_info.voltage > BATTERY_VOLTAGE_80 ? 100 : (battery_info.voltage > BATTERY_VOLTAGE_20 ? 50 : 10);

    // Calculate threshold levels taking hysteresis into account
    float battery_voltage_80 = BATTERY_VOLTAGE_80 - (battery_info.status == BATTERY_STATUS_HIGH ? BATTERY_VOLTAGE_80 * BATTERY_STATUS_HYSTERESIS : 0);
    float battery_voltage_20 = BATTERY_VOLTAGE_20 - (battery_info.status != BATTERY_STATUS_LOW ? BATTERY_VOLTAGE_20 * BATTERY_STATUS_HYSTERESIS : 0);

    if (battery_info.voltage > INPUT_VOLTAGE_MAX) {
        battery_info.status = BATTERY_STATUS_CHARGING;
    } else if (battery_info.voltage > battery_voltage_80) {
        battery_info.status = BATTERY_STATUS_HIGH;
    } else if (battery_info.voltage > battery_voltage_20) {
        battery_info.status = BATTERY_STATUS_MEDIUM;
    } else if (battery_info.voltage > (battery_voltage_20 * 0.2)) {
        battery_info.status = BATTERY_STATUS_LOW;
    } else {
        battery_info.status = BATTERY_STATUS_VERY_LOW;
    }
}

void check_battery() {
    battery_st_info last_battery_info = battery_info;
    update_battery_info();

    if (!(last_battery_info == battery_info)) {
        battery_signal.emit(&battery_info);
    }
}

} // namespace battery
