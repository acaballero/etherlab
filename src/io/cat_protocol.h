//
// Created by Angel Dust on 17/06/2024.
//

#ifndef TRX_FRONTEND_CAT_PROTOCOL_H
#define TRX_FRONTEND_CAT_PROTOCOL_H

#include "os/periodic_task.h"
#include "stdio.h"
#include "cat_if.h"

// iCOM  CI-V protocol
// See https://www.icomeurope.com/wp-content/uploads/2020/08/IC-705_ENG_CI-V_1_20200721.pdf

#define BROADCAST_ADDRESS 0x00  // Broadcast address
#define CONTROLLER_ADDRESS 0xE0 // Controller address
#define RADIO_ADDRESS 0xA4      // Radio address IC-705
//#define RADIO_ADDRESS        0x98 // Radio address IC-7610

#define START_BYTE 0xFE         // Start byte
#define STOP_BYTE CAT_STOP_BYTE // Stop byte

#define CMD_TRANS_FREQ 0x00 // Transfers operating frequency data
#define CMD_TRANS_MODE 0x01 // Transfers operating mode data

#define CMD_READ_FREQ 0x03 // Read operating frequency data
#define CMD_READ_MODE 0x04 // Read operating mode data

#define CMD_SET_FREQ 0x05 // Write operating frequency data
#define CMD_SET_MODE 0x06 // Write operating mode data

#define CMD_SET_VFO 0x07 // Write VFO

#define CMD_READ_SPLIT_MODE 0x0F // Read Split ON/OFF
#define CMD_READ_ATT 0x11

#define CMD_SET_VFO_A 0x00
#define CMD_SET_VFO_B 0x01
#define CMD_SET_VFO_AB 0xA0

// 0x1C commads
#define CMD_SEND_READ_MODE 0x00
#define CMD_SEND_READ_TUNER 0x01
#define CMD_SEND_READ_XFC 0x02
#define CMD_READ_TX_FREQ 0x03

// 0x15 commands
#define CMD_READ_SQUELCH_STATUS 0x01
#define CMD_READ_OTHER_SQUELCH_STATUS 0x05
#define CMD_READ_OVF_STATUS 0x07
#define CMD_READ_S_METER 0x02
#define CMD_READ_VCC 0x15

#define CMD_SET_VFO_FREQ 0x25 // Write VFO freq
#define CMD_SET_VFO_MODE 0x26 // Write VFO mode

#define CMD_ON_OFF 0x18 // Turn on/off the transceiver
#define SUBCMD_OFF 0x00
#define SUCBMD_ON 0x01

#define CMD_SEND_READ_LEVELS 0x14

#define CMD_EXTENDED 0x1A // Extended
#define SUBCMD_SET_READ_MEM 0x00
#define SUBCMD_READ_IF_WIDTH 0x03
#define SUBCMD_READ_AGC_TIME_CONSTANT 0x04
#define SUBCMD_SET 0x05
#define SUBCMD_SET_READ_REF 0x0089 // 0000=0% 0255=100%

#define IF_PASSBAND_WIDTH_WIDE 0x01
#define IF_PASSBAND_WIDTH_MEDIUM 0x02
#define IF_PASSBAND_WIDTH_NARROW 0x03

#define MODE_TYPE_LSB 0x00
#define MODE_TYPE_USB 0x01
#define MODE_TYPE_AM 0x02
#define MODE_TYPE_CW 0x03
#define MODE_TYPE_RTTY 0x04
#define MODE_TYPE_FM 0x05
#define MODE_TYPE_WFM 0x06

namespace cat_protocol {
typedef void (*usb_request_handler_fn)(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);
extern os::periodic_task task;
} // namespace cat_protocol

#endif // TRX_FRONTEND_CAT_PROTOCOL_H
