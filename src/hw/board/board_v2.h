//
// Created by Angel Dust on 29/05/2021.
//

#ifndef TRX_FRONTEND_BOARD_V2_H
#define TRX_FRONTEND_BOARD_V2_H

#define SI5351_PLLB_FREQ 80000000000ULL
#define SI5351_XTAL_FREQ 27000000
#define SI5351_RX_CLK SI5351_CLK7
#define SI5351_TX_CLK SI5351_CLK6
#define SI5351_2LO_CLK SI5351_CLK3
#define SI5351_IF_CLK SI5351_CLK1

#define ADF4351_XTAL_FREQ 27000000

#define SD_CARD_WRITE_MAX_KBPS 5000

#include "../../../lib/ADF4351/adf4351.h"
#include "../../../lib/CMX973/cmx973.h"
#include "../../../lib/Si5351/si5351_I2C.h"

#include "Signal.h"
#include "os/periodic_task.h"
#include "types.h"

// CMX973 allowed min gain value per amplifier
// VGA: -18dB
// VGB: -30dB
enum IF_GAIN { IF_GAIN_0, IF_GAIN_MINUS6, IF_GAIN_MINUS12, IF_GAIN_MINUS18, IF_GAIN_MINUS24, IF_GAIN_MINUS30 };

#define MIN_VGA_GAIN IF_GAIN_MINUS18
#define MIN_VGB_GAIN IF_GAIN_MINUS24

struct st_radio_config {
    RF_DIRECTION direction;
    uint64_t sample_freq;
    uint64_t freq = 0;
    IF_PROCESSING_MODE mode = ANALOG;
};

extern adf4350_init_param adf4350Params;
extern IF_GAIN vga_gain, vgb_gain;
extern Signal if_gain_signal;

void lo_strength(uint8_t stage, LO_POWER strength);
void lo_setup();
/**
 * Sets IF LO quadrature clocks
 * @param direction
 * @param freq
 */
void lo_enable(uint8_t stage, bool enabled);
bool if_freq(RF_DIRECTION direction, uint64_t freq, bool log = true);
void if_gain(RF_DIRECTION direction, IF_GAIN vga, IF_GAIN vgb);
void if_direction(RF_DIRECTION direction);
bool lo_freq(uint8_t stage, uint64_t freq);
void setup_board_peripherals();
void calibrate_freq();
int power_down_lo_clocks();
int power_up_lo_clocks();
int get_board_gain();
int get_if_gain();
int get_max_input_dbm();
/*
 * Configures the DSP hardware radio
 */
bool radio_config(st_radio_config);

namespace board {
extern bool change_drive_strength;
extern bool change_calibration;
extern os::periodic_task task;

int16_t if_gain_to_db(IF_GAIN if_gain);
} // namespace board

#endif // TRX_FRONTEND_BOARD_V2_H
