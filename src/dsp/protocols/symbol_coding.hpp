#ifndef __SYMBOL_CODING_H__
#define __SYMBOL_CODING_H__

#include <cstdint>
#include <cstddef>

namespace symbol_coding {

class NRZIDecoder {
  public:
    uint_fast8_t operator()(const uint_fast8_t symbol) {
        const auto out = (~(symbol ^ last)) & 1;
        last = symbol;
        return out;
    }

  private:
    uint_fast8_t last{0};
};

class ACARSDecoder {
  public:
    uint_fast8_t operator()(const uint_fast8_t symbol) {
        last ^= (~symbol & 1);
        return last;
    }

  private:
    uint_fast8_t last{0};
};

} /* namespace symbol_coding */

#endif /*__SYMBOL_CODING_H__*/
