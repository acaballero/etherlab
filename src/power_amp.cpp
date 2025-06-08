#include "power_amp.h"
#include "hw/stm32.h"
#include "os/periodic_task.h"
#include "config.h"

// Coefficients for the curve fitting the measured temperature (C) vs voltage(mV) at the detector
// This is using a 3.3k NTC shunt thermistor pulled up to 5.1v through 3.3k
// The temperature was measured with a thermocouple touching the top copper plane of the PCB at around 1cm of both drains.
// At 65º, the drains were at 100º
// 30	3.08
// 32	3.04
// 39	2.91
// 41	2.87
// 50	2.75
// 60	2.66
// 66	2.6
// We use a modified exponential curve y=a*e^(b/x) with the following coefficients

#define FITTING_COEF_A 11.65930841510168
#define FITTING_COEF_B 2.611004656928173

namespace power_amp {

// private forward declarations
void calculate_temp();

void check_temp();

void set_status(enum status);

bool enabled = false;
st_power_amp_params params;
float hysteresis = 0.94;
os::periodic_task task(2000, check_temp);
Signal temp_signal, status_signal;
enum status last_status = OFF, status = OFF;
float voltage;
int temp = params.MIN_TEMP - 1;
int curr_temp;
int prev_temp;
bool debug = false;

void enable() {
    enabled = true;
    check_temp();
    task.set_enabled(true);
}

void disable() {
    enabled = false;
    temp = params.MIN_TEMP - 1;
    set_status(OFF);
    task.set_enabled(false);
}

void test() {
    temp = 35 + ((HAL_GetTick() / 1000) % 100);
}

void calculate_temp() {
    uint16_t vadc = GetADCValue(&hadc3, POWER_AMP_TEMP_ADC_CHANNEL, 3);
    float v = ((float)vadc / (float)MAX_ADC_VALUE) * (float)V_REF;

    if (debug) {
        return test();
    }

    if (v > 1) {
        // filter for smoothness
        voltage = (voltage - (0.6 * (voltage - v)));

        prev_temp = curr_temp;

        // Voltage to temperature conversion by curve fitting
        curr_temp = round(FITTING_COEF_A * exp(FITTING_COEF_B / voltage));

        // Invalid values:
        // dt > 1ºC -> unstable
        // t < MIN_TEMP
        int dt = curr_temp - prev_temp;

        if (abs(dt) <= 1 && curr_temp >= params.MIN_TEMP) {
            temp = curr_temp;
        } else {
            temp = params.MIN_TEMP - 1; // invalid
        }
    } else {
        temp = params.MIN_TEMP - 1; // invalid
    }
}

void set_status(enum status s) {
    status = s;
    if (status != last_status) {
        status_signal.emit(&status);
        last_status = status;
    }
}

void check_temp() {
    calculate_temp();
    temp_signal.emit(&temp);

    int max = params.MAX_TEMP;
    if (status == HIGH_TEMP) {
        max *= hysteresis;
    }

    set_status(enabled ? temp < max ? OK : HIGH_TEMP : OFF);
}

} // namespace power_amp
