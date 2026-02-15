//
// Created by Angel Dust on 10/06/2021.
//

#ifndef TRX_FRONTEND_BOARD_V2_CONFIG_H
#define TRX_FRONTEND_BOARD_V2_CONFIG_H

#include "../stm32.h"
#include "board_v2.h"

typedef struct {

    // 2nd stage of gain of the quadrature demodulator
    IF_GAIN cmx973_vga = IF_GAIN_MINUS12;
    // 1st stage of gain of the quadrature demodulator
    IF_GAIN cmx973_vgb = IF_GAIN_0;

    uint32_t sd_write_max_kbps = SD_CARD_WRITE_MAX_KBPS;

    /* Since the input to the quadrature mixer is single ended from the DAC, and the negative port is biased from
     * a voltage divider (R40 & R45), the DAC needs to be biased at the same voltage in order to minimize the
     * carrier and image leakage at the mixer. This should have been done
     * by biasing each DAC channel in the board, but it is not :(. It happens that the common mode of the
     * DAC output (I and Q) gets biased at whatever level the signal has after DSP processing and, to drive it
     * to the proper common mode (that of the voltage divider biasing the negative input), we have to apply an offset here
     * TODO: Hack the board to bias the I/Q DAC channels coupling them to voltage dividers
     * EDIT: Did I do this already?
     * FIXME: The problem (in addition) is that the adc_type is int16 and DAC cannot be fed with negative values so a shift is needed
     */
    uint16_t dac_offset = 2035;

    // Difference in offset between I and Q DAC channels
    int16_t dac_off_balance = 13;
    // Ammplitude balance  between I and Q DAC channels
    float32_t dac_amp_balance = 1.01;

} st_hw_config;

#endif // TRX_FRONTEND_BOARD_V2_CONFIG_H
