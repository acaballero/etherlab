
#ifndef __APRS_H__
#define __APRS_H__

#include <cstring>
#include <string>

namespace aprs {

/*
 * Builds APRS frame
 * path: WIDE1-1,WIDE2-2 or null if no path should be added
 */
size_t build_frame(const char *src_address, const uint32_t src_ssid, const char *dest_address, const uint32_t dest_ssid, const std::string &payload,
                   const char *path, uint16_t *buffer);

} /* namespace aprs */

#endif /*__APRS_H__*/
