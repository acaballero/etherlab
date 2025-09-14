
#include "ax25.hpp"

namespace ax25 {

void AX25Frame::make_extended_field(char *const data, size_t length) {
    size_t i = 0;

    for (i = 0; i < length - 1; i++) {
        add_data(data[i] << 1);
    }

    add_data((data[i] << 1) | 1);
}

void AX25Frame::NRZI_add_bit(const uint32_t bit) {
    if (!bit) {
        current_bit ^= 1; // Zero: flip
    }

    current_byte <<= 1;
    current_byte |= current_bit;

    bit_counter++;

    if (bit_counter == 8) {
        bit_counter = 0;
        *data_ptr = current_byte;
        data_ptr++;
    }
}

void AX25Frame::add_byte(uint8_t byte, bool is_flag, bool is_data) {
    bool bit;

    if (is_data) {
        crc_ccitt.process_byte(byte);
    }

    for (uint32_t i = 0; i < 8; i++) {
        bit = (byte >> i) & 1;

        NRZI_add_bit(bit);

        if (bit) {
            ones_counter++;
            if ((ones_counter == 5) && (!is_flag)) {
                NRZI_add_bit(0);
                ones_counter = 0;
            }
        } else {
            ones_counter = 0;
        }
    }
}

void AX25Frame::flush() {
    if (bit_counter) {
        *data_ptr = current_byte << (8 - bit_counter); // Pad the last byte
    }
    data_ptr++;
    *data_ptr = 0;
};

void AX25Frame::add_flag() {
    add_byte(AX25_FLAG, true, false);
};

void AX25Frame::add_data(uint8_t byte) {
    add_byte(byte, false, true);
};

void AX25Frame::add_checksum() {
    auto checksum = crc_ccitt.checksum();
    add_byte(checksum, false, false);
    add_byte(checksum >> 8, false, false);
}

void AX25Frame::build(char *const address, const uint8_t control, const uint8_t protocol, const std::string &info, uint16_t *buffer) {
    size_t i;

    bit_counter = 0;
    current_bit = 0;
    current_byte = 0;
    ones_counter = 0;
    crc_ccitt.reset();
    data_ptr = buffer;

    add_flag();
    add_flag();
    add_flag();
    add_flag();

    make_extended_field(address, 14);
    add_data(control);
    add_data(protocol);

    for (i = 0; i < info.size(); i++) {
        add_data(info[i]);
    }

    add_checksum();

    add_flag();
    add_flag();

    flush();
}

} /* namespace ax25 */
