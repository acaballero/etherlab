//
// Created by Angel Dust on 18/05/2025.
//

#ifndef __APRS_PACKET_H__
#define __APRS_PACKET_H__

#include "dsp/dsp_common.h"
#include "hw/stm32f4xx/rtc.h"
namespace dsp {

constexpr int APRS_MIN_LENGTH = 18; // 14 bytes address, control byte and pid. 2 CRC.

struct aprs_pos {
    float latitude;
    float longitude;
    uint8_t symbol_code;
    uint8_t sym_table_id;
};

enum ADDRESS_TYPE { SOURCE, DESTINATION, REPEATER };

class APRSPacket {
  public:
    void set_timestamp(const st_datetime &value) {
        timestamp_ = value;
    }

    st_datetime timestamp() const {
        return timestamp_;
    }

    void set(const size_t index, const uint8_t data) {
        payload[index] = data;

        if (index + 1 > payload_size) {
            payload_size = index + 1;
        }
    }

    uint32_t operator[](const size_t index) const {
        return payload[index];
    }

    uint8_t size() const {
        return payload_size;
    }

    void set_valid_checksum(const bool valid) {
        valid_checksum = valid;
    }

    bool is_valid_checksum() const {
        return valid_checksum;
    }

    uint64_t get_source() {
        uint64_t source = 0x0;

        for (uint8_t i = SOURCE_START; i < SOURCE_START + ADDRESS_SIZE; i++) {
            source |= (((uint64_t)payload[i]) << ((i - SOURCE_START) * 8));
        }

        return source;
    }

    void get_source_formatted(char *buff) {
        parse_address(SOURCE_START, SOURCE);
        snprintf(buff, 15, "%s", address_buffer);
    }

    void get_destination_formatted(char *buff) {
        parse_address(DESTINATION_START, DESTINATION);
        snprintf(buff, 15, "%s", address_buffer);
    }

    std::string get_digipeaters_formatted() {
        uint8_t position = DIGIPEATER_START;
        bool has_more = parse_address(SOURCE_START, REPEATER);

        std::string repeaters = "";
        while (has_more) {
            has_more = parse_address(position, REPEATER);
            repeaters += std::string(address_buffer);

            position += ADDRESS_SIZE;

            if (has_more) {
                repeaters += ">";
            }
        }

        return repeaters;
    }

    uint8_t get_number_of_digipeaters() {
        uint8_t position = DIGIPEATER_START;
        bool has_more = parse_address(SOURCE_START, REPEATER);
        uint8_t repeaters = 0;
        while (has_more) {
            has_more = parse_address(position, REPEATER);
            position += ADDRESS_SIZE;
            repeaters++;
        }

        return repeaters;
    }

    uint8_t get_information_start_index() {
        return DIGIPEATER_START + (get_number_of_digipeaters() * ADDRESS_SIZE) + 2;
    }

    std::string get_information_text_formatted() {
        uint8_t start = get_information_start_index();
        if (start >= payload_size - 2) {
            return {};
        }

        return std::string(reinterpret_cast<char *>(&payload[start]), payload_size - start - 2);
    }

    void get_stream_text(std::string &stream) {

        std::string digis, info;

        char source[15], destination[15];
        get_source_formatted(source);
        get_destination_formatted(destination);
        digis = get_digipeaters_formatted();

        stream.reserve(80); // Optional optimization
        stream += source;
        stream += " > ";
        stream += destination;
        stream += " ; ";
        stream += digis;
        stream += " ; ";
        stream += get_information_text_formatted();
        ;
    }

    char get_data_type_identifier() {
        char ident = '\0';
        for (uint8_t i = get_information_start_index(); i < payload_size - 2; i++) {
            ident = payload[i];
            break;
        }
        return ident;
    }

    bool has_position() {
        char ident = get_data_type_identifier();

        return ident == '!' || ident == '=' || ident == '/' || ident == '@' || ident == ';' || ident == '`' || ident == '\'' || ident == 0x1d || ident == 0x1c;
    }

    aprs_pos get_position() {
        aprs_pos pos;

        char ident = get_data_type_identifier();
        std::string info_text = get_information_text_formatted();

        std::string lat_str, lng_str;
        char first;
        // bool supports_compression = true;
        bool is_mic_e_format = false;
        std::string::size_type start;

        switch (ident) {
            case '/':
            case '@':
                start = 8;
                break;
            case '=':
            case '!':
                start = 1;
                break;
            case ';':
                start = 18;
                // supports_compression = false;
                break;
            case '`':
            case '\'':
            case 0x1c:
            case 0x1d:
                is_mic_e_format = true;
                break;
            default:
                return pos;
        }
        if (is_mic_e_format) {
            parse_mic_e_format(pos);
        } else {
            if (start < info_text.size()) {
                first = info_text.at(start);

                if (std::isdigit(first)) {
                    if (start + 18 < info_text.size()) {
                        lat_str = info_text.substr(start, 8);
                        pos.sym_table_id = info_text.at(start + 8);
                        lng_str = info_text.substr(start + 9, 9);
                        pos.symbol_code = info_text.at(start + 18);

                        pos.latitude = parse_lat_str(lat_str);
                        pos.longitude = parse_lng_str(lng_str);
                    }

                } else {
                    if (start + 9 < info_text.size()) {
                        pos.sym_table_id = info_text.at(start);
                        lat_str = info_text.substr(start + 1, 4);
                        lng_str = info_text.substr(start + 5, 4);
                        pos.symbol_code = info_text.at(start + 9);

                        pos.latitude = parse_lat_str_cmp(lat_str);
                        pos.longitude = parse_lng_str_cmp(lng_str);
                    }
                }
            }
        }

        return pos;
    }

    void init_test_packet(const std::string &source, const std::string &destination, const std::string &info_text) {

        clear();

        auto encode_callsign = [](const std::string &callsign, uint8_t ssid, uint8_t data[7]) {
            for (size_t i = 0; i < 6; i++) {
                if (i < callsign.length()) {
                    data[i] = (callsign[i] << 1);
                } else {
                    data[i] = (' ' << 1);
                }
            }
            data[6] = ((ssid & 0x0F) << 1) | 0x60; // C bit = 1 (end of address), reserved bits
        };

        size_t index = 0;

        // Encode destination (SSID 0)
        uint8_t encoded[7];
        encode_callsign(destination, 0, encoded);
        for (uint8_t byte : encoded) {
            set(index++, byte);
        }

        // Encode source (SSID 0)
        encode_callsign(source, 0, encoded);
        for (uint8_t byte : encoded) {
            // Make sure the last byte has bit 0 = 1 to indicate end of address field
            byte |= 0x01;
            set(index++, byte);
        }

        // Control field (0x03) and PID (0xF0) for UI frame
        set(index++, 0x03);
        set(index++, 0xF0);

        // Create test position data (Madrid, Spain coordinates)
        // Format: !DDMM.mmN/DDDMM.mmW#
        // Madrid: 40.4168° N, 3.7038° W
        std::string position_info = "!4025.01N/00342.23W#Test APRS position packet";

        // If custom info_text is provided and not empty, append it
        if (!info_text.empty()) {
            position_info = "!4025.01N/00342.23W#" + info_text;
        }

        // Add position info to packet
        for (char c : position_info) {
            set(index++, static_cast<uint8_t>(c));
        }

        // Fake CRC (not validated here)
        set(index++, 0x00);
        set(index++, 0x00);

        set_valid_checksum(true);
    }

    void clear() {
        payload_size = 0;
    }

  private:
    const uint8_t DIGIPEATER_START = 14;
    const uint8_t SOURCE_START = 7;
    const uint8_t DESTINATION_START = 0;
    const uint8_t ADDRESS_SIZE = 7;

    bool valid_checksum = false;
    uint8_t payload[256];
    char address_buffer[15];
    uint8_t payload_size = 0;
    st_datetime timestamp_{};

    float parse_lat_str_cmp(const std::string &lat_str) {
        return 90.0 - ((lat_str.at(0) - 33) * (91 * 91 * 91) + (lat_str.at(1) - 33) * (91 * 91) + (lat_str.at(2) - 33) * 91 + (lat_str.at(3))) / 380926.0;
    }

    float parse_lng_str_cmp(const std::string &lng_str) {
        return -180.0 + ((lng_str.at(0) - 33) * (91 * 91 * 91) + (lng_str.at(1) - 33) * (91 * 91) + (lng_str.at(2) - 33) * 91 + (lng_str.at(3))) / 190463.0;
    }

    uint8_t parse_digits(const std::string &str) {
        if (str.at(0) == ' ') {
            return 0;
        }
        uint8_t end = str.find_last_not_of(' ') + 1;
        std::string sub = str.substr(0, end);

        if (!is_digits(sub)) {
            return 0;
        } else {
            return atoi(sub.c_str());
        }
    }

    bool is_digits(const std::string &str) {
        return str.find_last_not_of("0123456789") == std::string::npos;
    }

    float parse_lat_str(const std::string &lat_str) {
        float lat = 0.0;

        std::string str_lat_deg = lat_str.substr(0, 2);
        std::string str_lat_min = lat_str.substr(2, 2);
        std::string str_lat_hund = lat_str.substr(5, 2);
        std::string dir = lat_str.substr(7, 1);

        uint8_t lat_deg = parse_digits(str_lat_deg);
        uint8_t lat_min = parse_digits(str_lat_min);
        uint8_t lat_hund = parse_digits(str_lat_hund);

        lat += lat_deg;
        lat += (lat_min + (lat_hund / 100.0)) / 60.0;

        if (dir.c_str()[0] == 'S') {
            lat = -lat;
        }

        return lat;
    }

    float parse_lng_str(std::string &lng_str) {
        float lng = 0.0;

        std::string str_lng_deg = lng_str.substr(0, 3);
        std::string str_lng_min = lng_str.substr(3, 2);
        std::string str_lng_hund = lng_str.substr(6, 2);
        std::string dir = lng_str.substr(8, 1);

        uint8_t lng_deg = parse_digits(str_lng_deg);
        uint8_t lng_min = parse_digits(str_lng_min);
        uint8_t lng_hund = parse_digits(str_lng_hund);

        lng += lng_deg;
        lng += (lng_min + (lng_hund / 100.0)) / 60.0;

        if (dir.c_str()[0] == 'W') {
            lng = -lng;
        }

        return lng;
    }

    void parse_mic_e_format(aprs_pos &pos) {
        std::string lat_str = "";
        std::string lng_str = "";

        bool is_north = false;
        bool is_west = false;
        uint8_t lng_offset = 0;
        for (uint8_t i = DESTINATION_START; i < DESTINATION_START + ADDRESS_SIZE - 1; i++) {
            uint8_t ascii = payload[i] >> 1;

            lat_str += get_mic_e_lat_digit(ascii);

            if (i - DESTINATION_START == 3) {
                lat_str += ".";
            }
            if (i - DESTINATION_START == 3) {
                is_north = is_mic_e_lat_N(ascii);
            }
            if (i - DESTINATION_START == 4) {
                lng_offset = get_mic_e_lng_offset(ascii);
            }
            if (i - DESTINATION_START == 5) {
                is_west = is_mic_e_lng_W(ascii);
            }
        }
        if (is_north) {
            lat_str += "N";
        } else {
            lat_str += "S";
        }

        pos.latitude = parse_lat_str(lat_str);

        float lng = 0.0;
        uint8_t information_start = get_information_start_index() + 1;
        for (uint8_t i = information_start; i < information_start + 3 && i < payload_size - 2; i++) {
            uint8_t ascii = payload[i];

            if (i - information_start == 0) { // deg
                ascii -= 28;
                ascii += lng_offset;

                if (ascii >= 180 && ascii <= 189) {
                    ascii -= 80;
                } else if (ascii >= 190 && ascii <= 199) {
                    ascii -= 190;
                }

                lng += ascii;
            } else if (i - information_start == 1) { // min
                ascii -= 28;
                if (ascii >= 60) {
                    ascii -= 60;
                }
                lng += ascii / 60.0;
            } else if (i - information_start == 2) { // hundredth minutes
                ascii -= 28;

                lng += (ascii / 100.0) / 60.0;
            }
        }

        if (is_west) {
            lng = -lng;
        }

        pos.longitude = lng;
    }

    uint8_t get_mic_e_lat_digit(uint8_t ascii) {
        if (ascii >= '0' && ascii <= '9') {
            return ascii;
        }
        if (ascii >= 'A' && ascii <= 'J') {
            return ascii - 17;
        }
        if (ascii >= 'P' && ascii <= 'Y') {
            return ascii - 32;
        }
        if (ascii == 'K' || ascii == 'L' || ascii == 'Z') {
            return ' ';
        }

        return '\0';
    }

    bool is_mic_e_lat_N(uint8_t ascii) {
        if (ascii >= 'P' && ascii <= 'Z') {
            return true;
        }
        return false; // not technical definition, but the other case is invalid
    }

    bool is_mic_e_lng_W(uint8_t ascii) {
        if (ascii >= 'P' && ascii <= 'Z') {
            return true;
        }
        return false; // not technical definition, but the other case is invalid
    }

    uint8_t get_mic_e_lng_offset(uint8_t ascii) {
        if (ascii >= 'P' && ascii <= 'Z') {
            return 100;
        }
        return 0; // not technical definition, but the other case is invalid
    }

    bool parse_address(uint8_t start, ADDRESS_TYPE address_type) {
        uint8_t byte = 0;
        uint8_t has_more = false;
        uint8_t ssid = 0;
        uint8_t buffer_index = 0;

        for (uint8_t i = start; i < start + ADDRESS_SIZE && i < payload_size - 2; i++) {
            byte = payload[i];

            if (i - start == 6) {
                has_more = (byte & 0x1) == 0;
                ssid = (byte >> 1) & 0x0F;

                if (ssid != 0 || address_type == REPEATER) {
                    address_buffer[buffer_index++] = '-';

                    if (ssid < 10) {
                        address_buffer[buffer_index++] = '0' + ssid;
                        address_buffer[buffer_index++] = '\0';
                    } else {
                        address_buffer[buffer_index++] = '1';
                        address_buffer[buffer_index++] = '0' + ssid - 10;
                        address_buffer[buffer_index++] = '\0';
                    }
                } else {
                    address_buffer[buffer_index++] = '\0';
                }
            } else {
                byte >>= 1;

                if (byte != ' ') {
                    address_buffer[buffer_index++] = byte;
                }
            }
        }

        return has_more;
    }
};

} // namespace dsp

#endif /*__APRS_PACKET_H__*/
