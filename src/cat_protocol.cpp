//
// Created by Angel Dust on 17/06/2024.
//

#include "cat_protocol.h"
#include "config.h"
#include "radio.h"
#include "main_board.h"
#include "cat_if.h"
#include "standby.h"
#include "status.h"
#include "types.h"
#include "ui/frequency_memory_ui.h"
#include "usb/usbd_cdc_if.h"
#include "printf.h"
#include <sys/_stdint.h>

namespace cat_protocol {

void process_command_queue();
os::periodic_task task(100, process_command_queue);

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

void cmd_send_read_levels(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_send_read_functions(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_send_read_mode(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_read_meters(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_set_read_mem(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

void cmd_read_att(st_usb_cdc_command *command, uint8_t *response, uint8_t *size);

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
                                                    NULL,
                                                    cmd_read_split_mode_handler,
                                                    NULL,
                                                    cmd_read_att,
                                                    NULL,
                                                    NULL,
                                                    cmd_send_read_levels,
                                                    cmd_read_meters,
                                                    cmd_send_read_functions,
                                                    NULL,
                                                    cmd_on_off_handler,
                                                    NULL,
                                                    cmd_extended_handler,
                                                    NULL,
                                                    cmd_send_read_mode,
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

// os::periodic_task task(100, process_command_queue);

void print_hex(char *buf, int size) {
    char *byte = buf;
    char str[4];
    while (size > 0) {
        if (*byte == '\r')
            continue;
        snprintf_(str, sizeof(str), "%.2x", *byte);
        size--;
        printf_("%s ", str);
        byte++;
    }
    printf_("\n");
    printf_("%s\n", "...................");
}

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

    print_hex((char *)command->data, command->size);

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
                printf_("Command 0x%.2x (%d) not recognized\n", buf[4], buf[4]);
                memcpy(response, nok_response_data, 5);
            }
        }
    }

    response[size++] = STOP_BYTE;

    print_hex((char *)response, size);

    CDC_Transmit_HS(response, size);
}

void cmd_send_read_mode(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {
    uint8_t *buf = command->data;
    uint64_t bcdfreq = 0;
    switch (buf[5]) {
        case CMD_SEND_READ_MODE:
            if (command->size == 7) {
                main_board::setMode(buf[5] == 0 ? ANALOG_RX : ANALOG_TX);
            }
            response[(*size)++] = !ISTX ? 0x01 : 0x00;
            break;
        case CMD_SEND_READ_TUNER:
            response[(*size)++] = 0;
            break;
        case CMD_SEND_READ_XFC:
            response[(*size)++] = 0;
            break;
        case CMD_READ_TX_FREQ:

            bcdfreq = uint64_to_bcd(radio::get_frequency());
            memcpy(response + *size, &bcdfreq, sizeof(bcdfreq));
            *size += 5;
            break;
        default:
            response[(*size)++] = 0;
            break;
    }
}

void cmd_read_meters(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {
    uint8_t *buf = command->data;

    switch (buf[5]) {
        case CMD_READ_SQUELCH_STATUS:
            response[(*size)++] = config.squelch_level == 0 ? 0x00 : 0x01;
            break;
        case CMD_READ_OTHER_SQUELCH_STATUS:
            response[(*size)++] = 0;
            break;
        case CMD_READ_OVF_STATUS:
            response[(*size)++] = 0;
            break;
        case CMD_READ_S_METER:
            response[(*size)++] = 0x00;
            response[(*size)++] = 0x00;
            break;
        case CMD_READ_VCC:
            response[(*size)++] = 0x00;
            response[(*size)++] = 0x75;
            break;
        default:
            response[(*size)++] = 0x00;
            response[(*size)++] = 0x00;
            break;
    }
}

void cmd_read_freq_handler(st_usb_cdc_command *, uint8_t *response, uint8_t *size) {
    uint64_t bcdfreq = 0;

    // response[*size++] = buf[5];
    bcdfreq = uint64_to_bcd(radio::get_frequency());
    memcpy(response + *size, &bcdfreq, sizeof(bcdfreq));
    *size += 5;
}

void cmd_read_mode_handler(st_usb_cdc_command *, uint8_t *response, uint8_t *size) {
    response[(*size)++] = from_modulation_mode(main_board::getModulationMode());
}

void cmd_set_freq_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *) {
    radio::set_frequency(parse_freq(command->data + 5));
    memcpy(response, ok_response_data, 5);
}

void cmd_set_mode_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *) {

    main_board::setModulationMode(to_modulation_mode(command->data[5]), false);
    memcpy(response, ok_response_data, 5);
}

void cmd_set_vfo_handler(st_usb_cdc_command *, uint8_t *response, uint8_t *) {

    // TODO: Not implemented
    // response[*size++] = buf[5];
    memcpy(response, ok_response_data, 5);
}

void cmd_read_split_mode_handler(st_usb_cdc_command *, uint8_t *response, uint8_t *size) {
    response[(*size)++] = 0; // OFF
}

void cmd_read_att(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {

    if (command->size == 7) {
        // TODO: Set
    }

    response[(*size)++] = config.frontend_path == radio::FRONTEND_PATH_ATT ? 0x20 : 0x00;
}

void cmd_on_off_handler(st_usb_cdc_command *command, uint8_t *, uint8_t *) {
    uint8_t *buf = command->data;

    if (buf[5] == SUBCMD_OFF) {
        standby::sleep();
    } else {
        standby::wakeup();
    }
}

void cmd_subcmd_set(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {

    uint8_t *buf = command->data;
    uint16_t subcmd = (((uint16_t)buf[6] << 8) + buf[7]);
    switch (subcmd) {

        case SUBCMD_SET_READ_REF:
            response[(*size)++] = 0;
            response[(*size)++] = 0;
            break;
    }
}

void cmd_send_read_levels(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {

    uint8_t *buf = command->data;
    if (command->size == 8) {
        // echo the received data
        response[(*size)++] = buf[5];
        response[(*size)++] = buf[6];
    } else {
        response[(*size)++] = 0;
        response[(*size)++] = 0;
    }
}

void cmd_send_read_functions(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {

    uint8_t *buf = command->data;
    if (command->size == 7) {
        // echo the received data
        response[(*size)++] = buf[5];

    } else {
        response[(*size)++] = 0;
    }
}

void cmd_extended_handler(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {
    uint8_t *buf = command->data;
    switch (buf[5]) {
        case SUBCMD_READ_IF_WIDTH:
            response[(*size)++] = 0;
            break;
        case SUBCMD_SET:
            cmd_subcmd_set(command, response, size);
            break;
        case SUBCMD_SET_READ_MEM:
            cmd_set_read_mem(command, response, size);
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

void cmd_set_read_mem(st_usb_cdc_command *command, uint8_t *response, uint8_t *size) {

    uint8_t *buf = command->data;

    printf_("Read/set memory\n");
    if (command->size > 8) { // set
        memcpy(response, nok_response_data, 5);
    }

    st_freq_mem *mem = freq_memory::find_id(buf[7], buf[9]);

    if (!mem) {
        memcpy(response, nok_response_data, 5);
    }

    // Memory Group & ID (2 Bytes Each)
    response[(*size)++] = (mem->group >> 8) & 0xFF;
    response[(*size)++] = mem->group & 0xFF;
    response[(*size)++] = (mem->id >> 8) & 0xFF;
    response[(*size)++] = mem->id & 0xFF;

    // Split & Select Memory Setting (Default: OFF)
    response[(*size)++] = 0x00;

    // Frequency (BCD Format)
    uint64_t bcdfreq = 0;
    bcdfreq = uint64_to_bcd(mem->freq);
    memcpy(response + *size, &bcdfreq, sizeof(bcdfreq));
    *size += 5;

    // Mode & Filter (Mode Mapped to CI-V Code)
    uint8_t mode_civ = from_modulation_mode(mem->mode);

    response[(*size)++] = mode_civ;
    response[(*size)++] = 0x01; // Default Filter

    // Duplex & Tone (OFF)
    response[(*size)++] = 0x00;
    response[(*size)++] = 0x00;

    // Data mode (OFF)
    response[(*size)++] = 0x00;

    // Duplex and tone (OFF)
    response[(*size)++] = 0x00;

    // Digital squelch (OFF)
    response[(*size)++] = 0x00;

    // Repeater Tone (None)
    memset(&response[(*size)], 0x00, 3);
    *size += 3;

    // Repeater squelch (None)
    memset(&response[(*size)], 0x00, 3);
    *size += 3;

    // DTCS code (None)
    memset(&response[(*size)], 0x00, 3);
    *size += 3;

    // Digital code squelch setting (None)
    response[(*size)++] = 0x00;

    // Duple offset frequency setting (None)
    memset(&response[(*size)], 0x00, 3);
    *size += 3;

    // Call Sign Fields (8 characters each)
    memset(&response[(*size)], ' ', 8); // UR (Destination) Call Sign
    *size += 8;
    memset(&response[(*size)], ' ', 8); // R1 (Access repeater) Call Sign
    *size += 8;
    memset(&response[(*size)], ' ', 8); // R2 (Gateway/Link repeater) Call Sign
    *size += 8;

    // Memory Name (16 Chars, Padded)
    memset(&response[(*size)], ' ', 16);
    strncpy((char *)&response[(*size)], mem->name, 16);
    *size += 16;
}

void cmd_set_vfo_mode_handler(st_usb_cdc_command *command, uint8_t *, uint8_t *) {
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
