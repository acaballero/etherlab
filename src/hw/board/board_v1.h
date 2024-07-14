//
// Created by Angel Dust on 29/05/2021.
//

#ifndef TRX_FRONTEND_BOARD_V1_H
#define TRX_FRONTEND_BOARD_V1_H

#include "../../../lib/Si5351/si5351_I2C.h"
#include "types.h"

#define SI5351_XTAL_FREQ 25000000

#define SD_CARD_WRITE_MAX_KBPS 300

struct st_radio_config {
    RF_DIRECTION direction;
    uint64_t sample_freq;
};

extern Si5351 si5351;

void setup_board_peripherals();
void radio_config(st_radio_config radioConfig);

#endif //TRX_FRONTEND_BOARD_V1_H
