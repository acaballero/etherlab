
#ifndef __BASEBAND_PACKET_H__
#define __BASEBAND_PACKET_H__

#include "hw/stm32f4xx/rtc.h"
#include <cstddef>
#include <bitset>

namespace baseband {

class Packet {
  public:
    void set_timestamp(const st_datetime &value) {
        timestamp_ = value;
    }

    st_datetime timestamp() const {
        return timestamp_;
    }

    void add(const bool symbol) {
        if (count < capacity()) {
            data[count++] = symbol;
        }
    }

    uint_fast8_t operator[](const size_t index) const {
        return (index < size()) ? data[index] : 0;
    }

    size_t size() const {
        return count;
    }

    size_t capacity() const {
        return data.size();
    }

    void clear() {
        count = 0;
    }

  private:
    std::bitset<2560> data{};
    st_datetime timestamp_{};
    size_t count{0};
};

} /* namespace baseband */

#endif /*__BASEBAND_PACKET_H__*/
