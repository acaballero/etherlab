
#include <cstdint>

#include "aprs.hpp"
#include "ax25.hpp"

using namespace ax25;

namespace aprs {

// Uppercas
static inline char up(char c) {
    return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c;
}

/*
 * Encodes a 6-char callsign, padded with spaces
 */
static void encode_addr7(char out[7], const char *call6, uint8_t ssid) {
    // callsign padded to 6 chars with spaces, uppercase
    for (int i = 0; i < 6; i++) {
        char c = call6 && call6[i] ? up(call6[i]) : ' ';
        out[i] = static_cast<uint8_t>(c);
    }

    //  According to ax.25 doc section 2.2.13.x.x and 2.4.1.2
    //  SSID need bits 5.6 set, so later when shifted it will end up being 011xxxx0 (xxxx = SSID number)
    //  Notice that if need to signal usage of AX.25 V2.0, (dest_ssid | 112); (MSb will need to be set at the end)
    out[6] = static_cast<uint8_t>((ssid & 0x0F) | 0x30);
}

/*
 * Parse APRS address like "WIDE1-1,WIDE2-2" into digis;
   Returns count added
 */
static size_t append_path(char *addr, size_t max_digis, const char *path_csv) {
    if (!path_csv || !*path_csv) {
        return 0;
    }

    size_t added = 0;
    const char *p = path_csv;

    while (*p && added < max_digis) {
        // Skip separators/spaces
        while (*p == ' ' || *p == ',') {
            p++;
        }
        if (!*p) {
            break;
        }

        // Read token up to ',' or end
        char token[16] = {0};
        size_t tlen = 0;
        while (*p && *p != ',' && tlen + 1 < sizeof(token)) {
            token[tlen++] = *p++;
        }
        token[tlen] = 0;

        // Char array as a placeholder for CALL-SSID
        char call[7] = {' ', ' ', ' ', ' ', ' ', ' ', 0};
        uint8_t ssid = 0;

        // Find '-'
        char *dash = nullptr;
        for (size_t i = 0; i < tlen; i++) {
            if (token[i] == '-') {
                dash = &token[i];
                break;
            }
        }

        size_t call_len = dash ? (size_t)(dash - token) : tlen;
        if (call_len > 6) {
            call_len = 6;
        }

        for (size_t i = 0; i < call_len; i++) {
            call[i] = token[i];
        }

        if (dash && dash[1]) {
            // Simple atoi
            uint32_t v = 0;
            const char *s = dash + 1;
            while (*s >= '0' && *s <= '9') {
                v = (v * 10) + (uint32_t)(*s - '0');
                s++;
            }
            ssid = (uint8_t)(v & 0x0F);
        }

        encode_addr7(&addr[added * 7], call, ssid);
        added++;
    }

    return added;
}

size_t build_frame(const char *src_address, const uint32_t src_ssid, const char *dest_address, const uint32_t dest_ssid, const std::string &payload,
                   const char *path, uint16_t *buffer) {
    AX25Frame frame;

    constexpr size_t MAX_DIGIPEATERS = 4;
    char address[7 * (2 + MAX_DIGIPEATERS)] = {0};
    size_t addr_len = 0;

    encode_addr7(&address[addr_len], dest_address, (uint8_t)dest_ssid);
    addr_len += 7;
    encode_addr7(&address[addr_len], src_address, (uint8_t)src_ssid);
    addr_len += 7;

    //  According to ax.25 doc section 2.2.13.x.x and 2.4.1.2
    //  SSID need bits 5.6 set, so later when shifted it will end up being 011xxxx0 (xxxx = SSID number)
    const size_t digis = append_path(&address[addr_len], MAX_DIGIPEATERS, path);
    addr_len += digis * 7;

    return frame.build(address, addr_len, 0x03, protocol_id_t::NO_LAYER3, payload, buffer);
}

} /* namespace aprs */
