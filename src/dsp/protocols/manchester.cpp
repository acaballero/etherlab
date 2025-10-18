
#include "manchester.hpp"

#include "string_format.hpp"

size_t ManchesterBase::symbols_count() const {
    return packet.size() / 2;
}

DecodedSymbol ManchesterDecoder::operator[](const size_t index) const {
    const size_t encoded_index = index * 2;
    if ((encoded_index + 1) < packet.size()) {
        const auto value = packet[encoded_index + sense];
        const auto error = packet[encoded_index + 0] == packet[encoded_index + 1];
        return {value, error};
    } else {
        return {0, 1};
    }
}

DecodedSymbol BiphaseMDecoder::operator[](const size_t index) const {
    const size_t encoded_index = index * 2;
    if ((encoded_index + 1) < packet.size()) {
        const auto value = packet[encoded_index + 0] != packet[encoded_index + 1];
        const uint_fast8_t error = encoded_index ? (packet[encoded_index - 1] == packet[encoded_index + 0]) : 0;
        return {value, error};
    } else {
        return {0, 1};
    }
}

FormattedSymbols format_symbols(const ManchesterBase &decoder) {
    const size_t payload_length_decoded = decoder.symbols_count();
    const size_t payload_length_hex_characters = (payload_length_decoded + 3) / 4;
    const size_t payload_length_symbols_rounded = payload_length_hex_characters * 4;

    std::string hex_data;
    std::string hex_error;
    hex_data.reserve(payload_length_hex_characters);
    hex_error.reserve(payload_length_hex_characters);

    uint_fast8_t data = 0;
    uint_fast8_t error = 0;
    for (size_t i = 0; i < payload_length_symbols_rounded; i++) {
        const auto symbol = decoder[i];

        data <<= 1;
        data |= symbol.value;

        error <<= 1;
        error |= symbol.error;

        if ((i & 3) == 3) {
            hex_data += to_string_hex(data & 0xf, 1);
            hex_error += to_string_hex(error & 0xf, 1);
        }
    }

    return {hex_data, hex_error};
}

void manchester_encode(uint8_t *dest, uint8_t *src, const size_t length, const size_t sense) {
    uint8_t part = sense ? 0 : 0xFF;

    for (size_t c = 0; c < length; c++) {
        if ((src[c >> 3] << (c & 7)) & 0x80) {
            *(dest++) = part;
            *(dest++) = ~part;
        } else {
            *(dest++) = ~part;
            *(dest++) = part;
        }
    }
}
