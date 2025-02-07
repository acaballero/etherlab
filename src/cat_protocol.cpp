//
// Created by Angel Dust on 17/06/2024.
//

#include "cat_protocol.h"
#include "radio.h"
#include "main_board.h"
#include "cat_if.h"
#include "standby.h"
#include "status.h"
#include "usb/usbd_cdc_if.h"

namespace cat_protocol {

void process_command_queue();
periodic_task task(100, process_command_queue);

void cmd_read_freq_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_read_mode_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_set_freq_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_set_mode_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_set_vfo_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_read_split_mode_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_on_off_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_extended_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_set_vfo_freq_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_set_vfo_mode_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

uint8_t nok_response_data[] = {START_BYTE, START_BYTE, CONTROLLER_ADDRESS, RADIO_ADDRESS, 0xFB};
uint8_t ok_response_data[] = {START_BYTE, START_BYTE, CONTROLLER_ADDRESS, RADIO_ADDRESS, 0xFB};

static usb_request_handler_fn request_handlers[] = {NULL,
                                                    NULL,
                                                    NULL,
                                                    cmd_read_freq_handler,
                                                    cmd_read_mode_handler,
                                                    cmd_set_freq_handler,
                                                    cmd_set_mode_handler,
                                                    cmd_set_vfo_handler,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    cmd_read_split_mode_handler,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    cmd_on_off_handler,
                                                    NULL,
                                                    NULL,
                                                    cmd_extended_handler,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    NULL,
                                                    cmd_set_vfo_freq_handler,
                                                    cmd_set_vfo_mode_handler};

// periodic_task task(100, process_command_queue);

//    void print_hex(char *buf, int size) {
//        char *byte = buf;
//        char str[4];
//        while (size > 0) {
//            if (*byte == '\r') continue;
//            snprintf(str, sizeof(str), "%.2x", *byte);
//            size--;
//            printf("%s ", str);
//            byte++;
//        }
//        printf("\n");
//        printf("%s\n", "...................");
//    }

uint64_t uint64_to_bcd(uint64_t n) {

    uint64_t ret = 0;
    uint64_t d, x = n, i = 0;
    // The BCD is right-padded and, for each byte, the 4 LSB is the higher digit
    // E.g. 144000000 -> 00 00 00 44 01
    // We start from the LSD, but the uint64_t is little endian

    while (x > 0) {
        d = x / 10;
        ret = (ret >> 4) | ((x - d * 10) << 36);
        x = d;
        i++;
    }

    // Now we pad
    while (i < 10) {
        ret >>= 4;
        i++;
    }

    return ret;
}

uint64_t parse_freq(uint8_t *data) {
    // Example of frequency request payload
    // FE FE E0 42 03 <00 00 58 45 01> FD

    uint64_t freq = (data[0] & 0x0F) + (((data[0] >> 4) & 0x0F) * 10) + ((data[1] & 0x0F) * 100) + (((data[1] >> 4) & 0x0F) * 1000) +
                    ((data[2] & 0x0F) * 10000) + (((data[2] >> 4) & 0x0F) * 100000) + ((data[3] & 0x0F) * 1000000) + (((data[3] >> 4) & 0x0F) * 10000000) +
                    ((data[4] & 0x0F) * 100000000) + (((data[4] >> 4) & 0x0F) * 1000000000);

    return freq;
}

MODULATION_MODE to_modulation_mode(uint8_t mode) {
    switch (mode) {
        case MODE_TYPE_AM:
            return AM;
        case MODE_TYPE_FM:
            return FM;
        case MODE_TYPE_WFM:
            return WFM;
        case MODE_TYPE_LSB:
            return SSB_LSB;
        case MODE_TYPE_CW:
            return CW;
        case MODE_TYPE_USB:
        case MODE_TYPE_RTTY:
            return SSB_USB;
        default:
            return SSB_USB;
    }
}

uint8_t from_modulation_mode(MODULATION_MODE mode) {
    switch (mode) {
        case AM:
            return MODE_TYPE_AM;
        case FM:
            return MODE_TYPE_FM;
        case WFM:
            return MODE_TYPE_WFM;
        case SSB_LSB:
            return MODE_TYPE_LSB;
        case SSB_USB:
            return MODE_TYPE_USB;

        default:
            status::handleError(status::ST_ERROR, "Modulation mode not found");
            return 0;
    }
}

void process_command(st_usb_cdc_command *command) {

    // print_hex((char *) command->data, command->size);

    uint8_t *buf = command->data;

    // The response is the request ending with the device address
    uint8_t response[15] = {START_BYTE, START_BYTE, CONTROLLER_ADDRESS, RADIO_ADDRESS, buf[4]};
    uint8_t size = 5;

    if (buf[0] == START_BYTE && buf[1] == START_BYTE) {
        if (buf[2] == RADIO_ADDRESS || buf[2] == BROADCAST_ADDRESS) {
            usb_request_handler_fn handler = request_handlers[buf[4]];
            if (handler) {
                handler(command, response, &size);
            } else {
                memcpy(response, nok_response_data, 5);
            }
        }
    }

    response[size++] = STOP_BYTE;

    CDC_Transmit_HS(response, size);
}

void cmd_read_freq_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {
    uint64_t bcdfreq = 0;

    // response[*size++] = buf[5];
    bcdfreq = uint64_to_bcd(radio::get_frequency());
    memcpy(response + *size, &bcdfreq, sizeof(bcdfreq));
    *size += 5;
}

void cmd_read_mode_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {
    response[(*size)++] = from_modulation_mode(main_board::getModulationMode());
}

void cmd_set_freq_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {
    radio::set_frequency(parse_freq(command->data + 5));
    memcpy(response, ok_response_data, 5);
}

void cmd_set_mode_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {

    main_board::setModulationMode(to_modulation_mode(command->data[5]), false);
    memcpy(response, ok_response_data, 5);
}

void cmd_set_vfo_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {

    // TODO: Not implemented
    // response[*size++] = buf[5];
    memcpy(response, ok_response_data, 5);
}

void cmd_read_split_mode_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {
    response[(*size)++] = 0; // OFF
}

void cmd_on_off_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {
    uint8_t *buf = command->data;

    if (buf[5] == SUBCMD_OFF) {
        standby::sleep();
    } else {
        standby::wakeup();
    }
}

void cmd_extended_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {
    uint8_t *buf = command->data;
    switch (buf[5]) {
        case SUBCMD_READ_IF_WIDTH:
            response[(*size)++] = 0;
            break;
        default:
            memcpy(response, nok_response_data, 5);
            break;
    }
}

void cmd_set_vfo_freq_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {

    // TODO: Not implemented
    // uin8_t vfo = buf[5]; // VFO
    if (command->size > 8) { // Write
        radio::set_frequency(parse_freq(command->data + 6));
        memcpy(response, ok_response_data, 5);
    } else {
        uint64_t bcdfreq = 0;
        response[(*size)++] = 0; // Always VFO_A
        bcdfreq = uint64_to_bcd(radio::get_frequency());
        memcpy(response + *size, &bcdfreq, sizeof(bcdfreq));
        *size += 5;
    }

    // response[size++] = 0xFB;
}

void cmd_set_vfo_mode_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {
    // TODO: Not implemented
    // uin8_t vfo = buf[5]; // VFO
    main_board::setModulationMode(to_modulation_mode(command->data[6]), false);
    // response[*size++] = 0xFB;
}

void process_command_queue() {
    while (usb_rx_fifo.available()) {
        st_usb_cdc_command *command;
        usb_rx_fifo.consume(CAT_COMMAND_SIZE, (char **)&command);
        process_command(command);
    }
}

} // namespace cat_protocol
