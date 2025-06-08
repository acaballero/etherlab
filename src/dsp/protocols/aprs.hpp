
#ifndef __APRS_H__
#define __APRS_H__

#include <cstring>
#include <string>
#include <sys/_stdint.h>

namespace aprs {

void build_frame(const char *src_address, const uint32_t src_ssid, const char *dest_address, const uint32_t dest_ssid, const std::string &payload,
                 uint16_t *buffer);

} /* namespace aprs */

#endif /*__APRS_H__*/
