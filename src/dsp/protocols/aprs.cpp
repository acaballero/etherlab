
#include <cstdint>

#include "aprs.hpp"
#include "ax25.hpp"

using namespace ax25;

namespace aprs {

void build_frame(const char *src_address, const uint32_t src_ssid, const char *dest_address, const uint32_t dest_ssid, const std::string &payload,
                 uint16_t *buffer) {
    AX25Frame frame;

    char address[14] = {0};

    memcpy(&address[0], dest_address, 6);
    memcpy(&address[7], src_address, 6);
    // euquiq: According to ax.25 doc section 2.2.13.x.x and 2.4.1.2
    //  SSID need bits 5.6 set, so later when shifted it will end up being 011xxxx0 (xxxx = SSID number)
    //  Notice that if need to signal usage of AX.25 V2.0, (dest_ssid | 112); (MSb will need to be set at the end)
    address[6] = (dest_ssid | 48);
    address[13] = (src_ssid | 48);

    frame.build(address, 0x03, protocol_id_t::NO_LAYER3, payload, buffer);
}

} /* namespace aprs */
