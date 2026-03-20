//
// OOK brute-force presets (PortaPack Mayhem reference)
//

#ifndef TRX_FRONTEND_OOK_BRUTE_PRESETS_H
#define TRX_FRONTEND_OOK_BRUTE_PRESETS_H

#include <cstddef>
#include <cstdint>
#include <vector>

enum class OOKBruteProtocol : uint8_t {
    CAME_12 = 0,
    CAME_24,
    NICE_12,
    NICE_24,
    HOLTEK_HT12,
    PRINCETON_24,

    COUNT
};

struct OOKBrutePreset {
    const char *name;
    OOKBruteProtocol protocol;

    uint8_t data_bits;
    uint32_t chip_duration_us;
    uint16_t default_repeat;

    // Template made of '0', '1', and 'C' placeholders. Each 'C' consumes one data bit.
    const char *format;

    // Chip patterns expanded for each 'C' depending on bit value.
    const char *zero;
    const char *one;
};

// Keep this bounded: brute mode generates a chip-level sequence.
// The reference implementation uses 512 bits max.
static constexpr size_t OOK_BRUTE_MAX_CHIPS = 512;

static constexpr OOKBrutePreset OOK_BRUTE_PRESETS[] = {
    // Protocol indices match PortaPack's ui_ookbrute.cpp
    {
        "CAME 12",
        OOKBruteProtocol::CAME_12,
        12,
        333, // ~3k symbols/s => ~333us per chip
        2,
        "0000000000000000000000000000000000001CCCCCCCCCCCC0000",
        "011",
        "001",
    },
    {
        "CAME 24",
        OOKBruteProtocol::CAME_24,
        24,
        333,
        2,
        "0000000000000000000000000000000000001CCCCCCCCCCCCCCCCCCCCCCCC0000",
        "011",
        "001",
    },
    {
        "NICE 12",
        OOKBruteProtocol::NICE_12,
        12,
        680,
        2,
        "000000000000000000000000000000000000000001CCCCCCCCCCCC0000",
        "011",
        "001",
    },
    {
        "NICE 24",
        OOKBruteProtocol::NICE_24,
        24,
        680,
        2,
        "000000000000000000000000000000000000000001CCCCCCCCCCCCCCCCCCCCCCCC0000",
        "011",
        "001",
    },
    {
        "Holtek HT12",
        OOKBruteProtocol::HOLTEK_HT12,
        12,
        390,
        2,
        "0000000000000000000000000000000000001CCCCCCCCCCCC00000000000",
        "011",
        "001",
    },
    {
        "Princeton 24",
        OOKBruteProtocol::PRINCETON_24,
        24,
        450,
        6,
        "000000000000000000000000000000000000CCCCCCCCCCCCCCCCCCCCCCCC10000000",
        "1000",
        "1110",
    },
};

static_assert(sizeof(OOK_BRUTE_PRESETS) / sizeof(OOK_BRUTE_PRESETS[0]) == static_cast<size_t>(OOKBruteProtocol::COUNT),
              "OOK_BRUTE_PRESETS mismatch");

static inline const OOKBrutePreset *ook_brute_get_preset(OOKBruteProtocol proto) {
    const size_t ix = static_cast<size_t>(proto);
    if (ix >= static_cast<size_t>(OOKBruteProtocol::COUNT)) {
        return nullptr;
    }
    return &OOK_BRUTE_PRESETS[ix];
}

static inline uint32_t ook_brute_max_code(const OOKBrutePreset &preset) {
    if (preset.data_bits == 0 || preset.data_bits >= 32) {
        return 0xFFFFFFFFu;
    }
    return (1u << preset.data_bits) - 1u;
}

static inline bool ook_brute_build_sequence(const OOKBrutePreset &preset, uint32_t counter, std::vector<uint8_t> &out) {

    out.clear();
    out.reserve(OOK_BRUTE_MAX_CHIPS);

    uint8_t cdb = 0; // current data bit index

    const char *fmt = preset.format;
    for (size_t i = 0; fmt[i] != '\0'; i++) {
        const char c = fmt[i];

        auto append_bit = [&](uint8_t b) {
            if (out.size() >= OOK_BRUTE_MAX_CHIPS) {
                return false;
            }
            out.push_back(b ? 1 : 0);
            return true;
        };

        auto append_pattern = [&](const char *pat) {
            for (size_t j = 0; pat[j] != '\0'; j++) {
                const char pc = pat[j];
                if (pc == '0') {
                    if (!append_bit(0)) {
                        return false;
                    }
                } else if (pc == '1') {
                    if (!append_bit(1)) {
                        return false;
                    }
                } else {
                    // invalid
                    return false;
                }
            }
            return true;
        };

        if (c == '0') {
            if (!append_bit(0)) {
                return false;
            }
        } else if (c == '1') {
            if (!append_bit(1)) {
                return false;
            }
        } else if (c == 'C') {
            if (cdb >= preset.data_bits) {
                return false;
            }
            const uint32_t mask = 1u << (preset.data_bits - cdb - 1);
            const bool bit = (counter & mask) != 0;
            if (!append_pattern(bit ? preset.one : preset.zero)) {
                return false;
            }
            cdb++;
        } else {
            // invalid character
            return false;
        }
    }

    // Accept formats that consume fewer bits, but reject those that overflow.
    return true;
}

#endif // TRX_FRONTEND_OOK_BRUTE_PRESETS_H
