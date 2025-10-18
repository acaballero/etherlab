#ifndef __FIELD_READER_H__
#define __FIELD_READER_H__

#include <cstdint>
#include <cstddef>

struct BitRemapNone {
    constexpr size_t operator()(const size_t &bit_index) const {
        return bit_index;
    }
};

struct BitRemapByteReverse {
    constexpr size_t operator()(const size_t bit_index) const {
        return bit_index ^ 7;
    }
};

template <typename T, typename BitRemap> class FieldReader {
  public:
    constexpr FieldReader(const T &data) : data{data} {
    }

    /* The "start_bit" winds up being the MSB of the returned field value. */
    /* The BitRemap functor determines which bits are read from the source
     * packet. */
    int32_t read(const size_t start_bit,
                 const size_t length) const { // Euquiq: was uint32_t, used for calculating lat / lon in radiosondes, can be negative too
        uint32_t value = 0;
        for (size_t i = start_bit; i < (start_bit + length); i++) {
            value = (value << 1) | data[bit_remap(i)];
        }
        return value;
    }

  private:
    const T &data;
    const BitRemap bit_remap{};
};

#endif /*__FIELD_READER_H__*/
