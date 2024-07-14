//
// Created by Angel Dust on 29/05/2021.
//

#include "stdio.h"
#include "../stm32.h"
#include "board_v1.h"
#include "../../config.h"
#include "../../../lib/Si5351/si5351_I2C.h"
#include "radio.h"

Si5351 si5351;

void change_lo_strenght() {
    si5351.drive_strength(SI5351_CLK2, (si5351_drive) config.lo_drive_strength_0);
}

void calibrate_freq() {
    si5351.set_correction(config.f_correction, SI5351_PLL_INPUT_XO);
    si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
}

void lo_freq(uint64_t freq) {

    __disable_irq();

#if DEBUG
    serial.print("setting si5351 to ");

    serial.println(f_freq);
#endif

    unsigned long long f = (unsigned long long) freq * SI5351_FREQ_MULT;
    uint8_t last_harmonic_mode = harmonic_mode;

    // For frequencies above SI5351_MULTISYNTH_MAX_FREQ, we use the third harmonic

    if (freq > SI5351_MULTISYNTH_MAX_FREQ) {
        f /= 3;
        harmonic_mode = true;
    } else {
        harmonic_mode = false;
    }

    // Only change the 70cm amplifier path if the harmonic mode has changed, to avoid setting the GPIO expansion pins unnecessarily when sweeping the frequency
    if (last_harmonic_mode != harmonic_mode) {
        setGPIOExpPin(&hmcp01, MCP23017_PORTA, GPIOEXP_70CM_AMP, harmonic_mode);
        setGPIOExpPin(&hmcp01, MCP23017_PORTA, GPIOEXP_70CM_AMP_BYPASS, !harmonic_mode);
    }

    if (si5351.clk_freq[(uint8_t) SI5351_CLK2] != f) {

        si5351.set_freq(f, SI5351_CLK2);
    }

    __enable_irq();
}


void if_freq() {

    if (config.enable_quadrature) {

        // If quadrature is enabled, we need to set the PLL frequency manually, because it must be
        // an even multiple of the desired frequency. This is so because the phase register value
        // is units of 1/4 the PLL period.

        // If you need a 90 degree phase shift it is quite easy to determine your parameters.
        // Pick a PLL frequency that is an even multiple of your clock frequency
        // (remember that the PLL needs to be in the range of 600 to 900 MHz).
        // Then to set a 90 degree phase shift, you simply enter that multiple into the phase register.
        // Remember when setting multiple outputs to be phase-related to each other,
        // they each need to be referenced to the same PLL.

        unsigned long long f_q = (unsigned long long) f_iq * SI5351_FREQ_MULT;


        int m = get_pll_multiple(f_iq);

        r = si5351.set_freq_manual((uint64_t) f_q, (uint64_t) (m * f_iq * SI5351_FREQ_MULT), SI5351_CLK0);

        if (!r) {

            si5351.set_freq_manual((uint64_t) f_q, (uint64_t) (m * f_iq * SI5351_FREQ_MULT), SI5351_CLK1);

            si5351.set_phase(SI5351_CLK0, 0);
            si5351.set_phase(SI5351_CLK1, m);

            if (pll_multiple != m) { // If the multiple has changed, the PLL needs to be reset
                pll_multiple = m;
                si5351.pll_reset(SI5351_PLLB);
            }

#if DEBUG
            serial.print("si5351 is set to ");
            serial.println(f_freq);
#endif

        } else {
#if DEBUG
            serial.println("Error setting si5351 set_freq_manual");
#endif
        }
    }
}


void radio_config(st_radio_config radio_config) {

    if (radio_config.direction==RF_DIRECTION_TX) {

        ADC_DMA_Stop(&hadc1);

        MX_DAC_Init();
        set_timer_sample_rate(DAC_TIMER, DAC_TIMER_CLOCK_HZ, radio_config.sample_freq);
        DAC_DMA_Start(&hdac1);

    }
    else {

        DAC_DMA_Stop(&hdac1);
        HAL_DAC_DeInit(&hdac1);
        ADC_DMA_Start(&hadc1);

    }

    if_direction(radio_config.direction);
}

void setup_board_peripherals() {

    // si5351
    bool b = si5351.init(hi2c1, SI5351_CRYSTAL_LOAD_10PF, SI5351_XTAL_FREQ, config.f_correction, 0);

    if (!b) {

#if LCD_ENABLED

        printf("si5351 init FAULT");

#endif

    } else {

        if (config.enable_quadrature) {

            si5351.set_ms_source(SI5351_CLK0, SI5351_PLLB);
            si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
            si5351.set_ms_source(SI5351_CLK1, SI5351_PLLB);
            si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_2MA);

#if DEBUG
            serial.println("si5351 drive strength: ");
            serial.println(config.drive_strength);
#endif

        } else {

            si5351.set_clock_disable(SI5351_CLK0, SI5351_CLK_DISABLE_LOW);
            si5351.output_enable(SI5351_CLK0, 0);
            si5351.set_clock_disable(SI5351_CLK1, SI5351_CLK_DISABLE_LOW);
            si5351.output_enable(SI5351_CLK1, 0);

        }

        si5351.drive_strength(SI5351_CLK2, (si5351_drive) config.drive_strength);
        //si5351.update_freq(1000000000ULL, SI5351_CLK2);
        set_freq();

    }
}