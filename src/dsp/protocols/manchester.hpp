
#ifndef __MANCHESTER_H__
#define __MANCHESTER_H__

#include <cstdint>
#include <cstddef>
#include <bitset>
#include <string>

#include "baseband_packet.hpp"

struct DecodedSymbol {
    uint_fast8_t value;
    uint_fast8_t error;
};

class ManchesterBase {
  public:
    constexpr ManchesterBase(const baseband::Packet &packet, const size_t sense = 0) : packet{packet}, sense{sense} {
    }

    virtual DecodedSymbol operator[](const size_t index) const = 0;

    virtual size_t symbols_count() const;

    virtual ~ManchesterBase(){};

  protected:
    const baseband::Packet &packet;
    const size_t sense;
};

class ManchesterDecoder : public ManchesterBase {
  public:
    using ManchesterBase::ManchesterBase;
    DecodedSymbol operator[](const size_t index) const;
};

class BiphaseMDecoder : public ManchesterBase {
  public:
    using ManchesterBase::ManchesterBase;
    DecodedSymbol operator[](const size_t index) const;
};

template <typename T> T operator|(const T &l, const DecodedSymbol &r) {
    return l | r.value;
}

struct FormattedSymbols {
    const std::string data;
    const std::string errors;
};

FormattedSymbols format_symbols(const ManchesterBase &decoder);

void manchester_encode(uint8_t *dest, uint8_t *src, const size_t length, const size_t sense = 0);

#endif /*__MANCHESTER_H__*/
