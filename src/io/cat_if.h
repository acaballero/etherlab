//
// Created by Angel Dust on 17/06/2024.
//

#ifndef TRX_FRONTEND_CAT_IF_H
#define TRX_FRONTEND_CAT_IF_H

#include "stdint.h"

#define CAT_COMMAND_SIZE 100
#define CAT_STOP_BYTE 0xFD

struct st_usb_cdc_command {
    uint8_t data[CAT_COMMAND_SIZE - 2]; // The size of the FIFO block minus the size of the uint16
    uint16_t size;
};

#ifdef __cplusplus
#include "FIFO.h"
extern "C" {
#endif

uint8_t cat_enqueue_command(char *buf, uint16_t len);

#ifdef __cplusplus
}
extern FIFO usb_rx_fifo;
#endif

#endif // TRX_FRONTEND_CAT_IF_H
