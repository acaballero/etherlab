//
// Created by Angel Dust on 17/06/2024.
//

#include "cat_if.h"
#include "FIFO.h"
#include "memory.h"

char *usb_rx_fifo_buffer[CAT_COMMAND_SIZE * 4];
FIFO usb_rx_fifo((char *)usb_rx_fifo_buffer, CAT_COMMAND_SIZE);
struct st_usb_cdc_command command;

uint8_t cat_enqueue_command(char *buf, uint16_t len) {

    /* The CDC HAL processes packets of 64 bytes. If a command is bigger, we will receive multiple chunks */
    FIFO_ERROR ret = FIFO_ERROR_NONE;

    if (command.size + len < CAT_COMMAND_SIZE - 2) {

        memcpy(command.data + command.size, buf, len);
        command.size += len;

        if ((uint8_t)buf[len - 1] == CAT_STOP_BYTE) { // Last byte received
            ret = usb_rx_fifo.write_block((char *)&command, CAT_COMMAND_SIZE);
            command.size = 0;
        }
    } else {
        ret = FIFO_ERROR_OVERRUN;
        command.size = 0;
    }
    return ret == FIFO_ERROR_NONE;
}
