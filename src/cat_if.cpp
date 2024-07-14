//
// Created by Angel Dust on 17/06/2024.
//

#include "cat_if.h"
#include "memory.h"

char * usb_rx_fifo_buffer[CAT_COMMAND_SIZE*4];
FIFO usb_rx_fifo((char *) usb_rx_fifo_buffer, CAT_COMMAND_SIZE);

uint8_t cat_enqueue_command(char *buf, uint16_t len) {

    struct st_usb_cdc_command command;
    memcpy(command.data, buf, len);
    command.size = len;

    FIFO_ERROR ret = usb_rx_fifo.writeBlock((char *)&command,CAT_COMMAND_SIZE);
    return ret == FIFO_ERROR_NONE;
}