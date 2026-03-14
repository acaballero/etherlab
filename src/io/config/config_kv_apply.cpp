// Created by ECA on 2026-03-14.

#include "config_kv_apply.h"

#include "types.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

namespace {

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool parse_u64_strict(const char *s, uint64_t *out) {
    if (!s || !out) {
        return false;
    }
    while (*s == ' ' || *s == '\t') {
        ++s;
    }
    if (*s == '+') {
        ++s;
    }
    if (!is_digit(*s)) {
        return false;
    }
    *out = safe_atou64(s);
    return true;
}

static bool parse_i64_strict(const char *s, int64_t *out) {
    if (!s || !out) {
        return false;
    }
    while (*s == ' ' || *s == '\t') {
        ++s;
    }
    if (*s == '+' || *s == '-') {
        ++s;
    }
    if (!is_digit(*s)) {
        return false;
    }
    *out = safe_atoi64(s);
    return true;
}

static bool parse_f32_strict(const char *s, float *out) {
    if (!s || !out) {
        return false;
    }
    while (*s == ' ' || *s == '\t') {
        ++s;
    }

    const char *p = s;
    if (*p == '+' || *p == '-') {
        ++p;
    }

    // Accept:
    // - digits...
    // - .digits...
    if (!(is_digit(*p) || (*p == '.' && is_digit(*(p + 1))))) {
        return false;
    }

    *out = strtof(s, nullptr);
    return true;
}

static bool parse_bool_strict(const char *s, bool *out) {
    int64_t tmp = 0;
    if (!parse_i64_strict(s, &tmp) || !out) {
        return false;
    }
    *out = (tmp != 0);
    return true;
}

} // namespace

namespace io::config_kv {

static const char *skip_ws(const char *s) {
    while (s && (*s == ' ' || *s == '\t')) {
        ++s;
    }
    return s;
}

bool apply_kv(const char *key, const char *value, st_config *cfg) {

    if (!key || !value || !cfg) {
        return false;
    }

    // Special: version is a string.
    if (std::strcmp(key, "version") == 0) {
        value = skip_ws(value);
        if (*value == 0) {
            return false;
        }
        std::strncpy(cfg->version, value, sizeof(cfg->version) - 1);
        cfg->version[sizeof(cfg->version) - 1] = 0;
        return true;
    }

    // vfo[<ix>].<field>
    // IMPORTANT: use %n + end-of-string check so that "vfo[1].step" does not
    // match the "vfo[%u].freq" pattern (sscanf would otherwise return 1 after
    // assigning the index).
    unsigned ix = 0;
    int n = 0;

    if (std::sscanf(key, "vfo[%u].freq%n", &ix, &n) == 1 && key[n] == 0) {
        if (ix < (sizeof(cfg->vfo) / sizeof(cfg->vfo[0]))) {
            uint64_t tmp = 0;
            if (!parse_u64_strict(value, &tmp)) {
                return false;
            }
            cfg->vfo[ix].freq = (uint32_t)tmp;
            return true;
        }
        return false;
    }

    n = 0;
    if (std::sscanf(key, "vfo[%u].step%n", &ix, &n) == 1 && key[n] == 0) {
        if (ix < (sizeof(cfg->vfo) / sizeof(cfg->vfo[0]))) {
            uint64_t tmp = 0;
            if (!parse_u64_strict(value, &tmp)) {
                return false;
            }
            cfg->vfo[ix].step = (uint32_t)tmp;
            return true;
        }
        return false;
    }

    n = 0;
    if (std::sscanf(key, "vfo[%u].rit%n", &ix, &n) == 1 && key[n] == 0) {
        if (ix < (sizeof(cfg->vfo) / sizeof(cfg->vfo[0]))) {
            int64_t tmp = 0;
            if (!parse_i64_strict(value, &tmp)) {
                return false;
            }
            cfg->vfo[ix].rit = (int32_t)tmp;
            return true;
        }
        return false;
    }

    n = 0;
    if (std::sscanf(key, "vfo[%u].modulation%n", &ix, &n) == 1 && key[n] == 0) {
        if (ix < (sizeof(cfg->vfo) / sizeof(cfg->vfo[0]))) {
            int64_t tmp = 0;
            if (!parse_i64_strict(value, &tmp)) {
                return false;
            }
            cfg->vfo[ix].mode = (MODULATION_MODE)tmp;
            return true;
        }
        return false;
    }

#define CFG_VERSION(ID) ((void)0)
#define CFG_VFO_ARRAY(ID) ((void)0)
#define CFG_FFT_IQ_BLOBS(ID) ((void)0)

#define CFG_MATCH_RET(KEYSTR, EXPR)                                                                                                                            \
    do {                                                                                                                                                       \
        if (std::strcmp(key, (KEYSTR)) == 0) {                                                                                                                 \
            return (EXPR);                                                                                                                                     \
        }                                                                                                                                                      \
    } while (0)

#define CFG_MATCH_SET(KEYSTR, TARGET, EXPR)                                                                                                                    \
    do {                                                                                                                                                       \
        if (std::strcmp(key, (KEYSTR)) == 0) {                                                                                                                 \
            (TARGET) = (EXPR);                                                                                                                                 \
            return true;                                                                                                                                       \
        }                                                                                                                                                      \
    } while (0)

#define CFG_BOOL(KEYSTR, ID, MEMBER) CFG_MATCH_RET(KEYSTR, parse_bool_strict(value, &cfg->MEMBER))

#define CFG_U8(KEYSTR, ID, MEMBER)                                                                                                                             \
    do {                                                                                                                                                       \
        if (std::strcmp(key, (KEYSTR)) == 0) {                                                                                                                 \
            uint64_t tmp = 0;                                                                                                                                  \
            if (!parse_u64_strict(value, &tmp)) {                                                                                                              \
                return false;                                                                                                                                  \
            }                                                                                                                                                  \
            cfg->MEMBER = (uint8_t)tmp;                                                                                                                        \
            return true;                                                                                                                                       \
        }                                                                                                                                                      \
    } while (0)

#define CFG_I8(KEYSTR, ID, MEMBER)                                                                                                                             \
    do {                                                                                                                                                       \
        if (std::strcmp(key, (KEYSTR)) == 0) {                                                                                                                 \
            int64_t tmp = 0;                                                                                                                                   \
            if (!parse_i64_strict(value, &tmp)) {                                                                                                              \
                return false;                                                                                                                                  \
            }                                                                                                                                                  \
            cfg->MEMBER = (int8_t)tmp;                                                                                                                         \
            return true;                                                                                                                                       \
        }                                                                                                                                                      \
    } while (0)

#define CFG_U16(KEYSTR, ID, MEMBER)                                                                                                                            \
    do {                                                                                                                                                       \
        if (std::strcmp(key, (KEYSTR)) == 0) {                                                                                                                 \
            uint64_t tmp = 0;                                                                                                                                  \
            if (!parse_u64_strict(value, &tmp)) {                                                                                                              \
                return false;                                                                                                                                  \
            }                                                                                                                                                  \
            cfg->MEMBER = (uint16_t)tmp;                                                                                                                       \
            return true;                                                                                                                                       \
        }                                                                                                                                                      \
    } while (0)

#define CFG_I16(KEYSTR, ID, MEMBER)                                                                                                                            \
    do {                                                                                                                                                       \
        if (std::strcmp(key, (KEYSTR)) == 0) {                                                                                                                 \
            int64_t tmp = 0;                                                                                                                                   \
            if (!parse_i64_strict(value, &tmp)) {                                                                                                              \
                return false;                                                                                                                                  \
            }                                                                                                                                                  \
            cfg->MEMBER = (int16_t)tmp;                                                                                                                        \
            return true;                                                                                                                                       \
        }                                                                                                                                                      \
    } while (0)

#define CFG_I32(KEYSTR, ID, MEMBER)                                                                                                                            \
    do {                                                                                                                                                       \
        if (std::strcmp(key, (KEYSTR)) == 0) {                                                                                                                 \
            int64_t tmp = 0;                                                                                                                                   \
            if (!parse_i64_strict(value, &tmp)) {                                                                                                              \
                return false;                                                                                                                                  \
            }                                                                                                                                                  \
            cfg->MEMBER = (int32_t)tmp;                                                                                                                        \
            return true;                                                                                                                                       \
        }                                                                                                                                                      \
    } while (0)

#define CFG_U32(KEYSTR, ID, MEMBER)                                                                                                                            \
    do {                                                                                                                                                       \
        if (std::strcmp(key, (KEYSTR)) == 0) {                                                                                                                 \
            uint64_t tmp = 0;                                                                                                                                  \
            if (!parse_u64_strict(value, &tmp)) {                                                                                                              \
                return false;                                                                                                                                  \
            }                                                                                                                                                  \
            cfg->MEMBER = (uint32_t)tmp;                                                                                                                       \
            return true;                                                                                                                                       \
        }                                                                                                                                                      \
    } while (0)

#define CFG_U32_L(KEYSTR, ID, MEMBER) CFG_U32(KEYSTR, ID, MEMBER)

#define CFG_FLOAT(KEYSTR, ID, MEMBER)                                                                                                                          \
    do {                                                                                                                                                       \
        if (std::strcmp(key, (KEYSTR)) == 0) {                                                                                                                 \
            float tmp = 0;                                                                                                                                     \
            if (!parse_f32_strict(value, &tmp)) {                                                                                                              \
                return false;                                                                                                                                  \
            }                                                                                                                                                  \
            cfg->MEMBER = tmp;                                                                                                                                 \
            return true;                                                                                                                                       \
        }                                                                                                                                                      \
    } while (0)

#define CFG_ENUM(KEYSTR, ID, MEMBER)                                                                                                                           \
    do {                                                                                                                                                       \
        if (std::strcmp(key, (KEYSTR)) == 0) {                                                                                                                 \
            int64_t tmp = 0;                                                                                                                                   \
            if (!parse_i64_strict(value, &tmp)) {                                                                                                              \
                return false;                                                                                                                                  \
            }                                                                                                                                                  \
            cfg->MEMBER = (std::decay_t<decltype(cfg->MEMBER)>)tmp;                                                                                            \
            return true;                                                                                                                                       \
        }                                                                                                                                                      \
    } while (0)

#include "config_schema.def"

#undef CFG_ENUM
#undef CFG_FLOAT
#undef CFG_U32_L
#undef CFG_U32
#undef CFG_I32
#undef CFG_I16
#undef CFG_U16
#undef CFG_I8
#undef CFG_U8
#undef CFG_BOOL

#undef CFG_MATCH_SET
#undef CFG_MATCH_RET

#undef CFG_FFT_IQ_BLOBS
#undef CFG_VFO_ARRAY
#undef CFG_VERSION

    return false;
}

} // namespace io::config_kv
