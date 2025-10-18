#ifndef __BIT_PATTERN_H__
#define __BIT_PATTERN_H__

#include <cstdint>
#include <cstddef>

class BitHistory {
  public:
    void add(const uint_fast8_t bit) {
        history = (history << 1) | (bit & 1);
    }

    uint64_t value() const {
        return history;
    }

  private:
    uint64_t history{0};
};

class BitPattern {
  public:
    constexpr BitPattern() : code_{0}, mask_{0}, maximum_hanning_distance_{0} {
    }

    constexpr BitPattern(const uint64_t code, const size_t code_length, const size_t maximum_hanning_distance = 0)
        : code_{code}, mask_{(1ULL << code_length) - 1ULL}, maximum_hanning_distance_{maximum_hanning_distance} {
    }

    bool operator()(const BitHistory &history, const size_t) const {
        const auto delta_bits = (history.value() ^ code_) & mask_;
        const size_t count = __builtin_popcountll(delta_bits);
        return (count <= maximum_hanning_distance_);
    }

  private:
    uint64_t code_;
    uint64_t mask_;
    size_t maximum_hanning_distance_;
};

#endif /*__BIT_PATTERN_H__*/
