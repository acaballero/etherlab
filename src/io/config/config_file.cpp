//
// Created by Angel Dust on 05/09/2025.
//
#include "config_file.h"
#include "dsp/aprs/aprs_settings.h"
#include "fatfs/fatfs.h"
#include "config.h"
#include "ff.h"
#include "config_kv_apply.h"
#include "printf.h"
#include "stm32f4xx_hal.h"
#include "types.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

template class ConfigFile<st_config>;
template class ConfigFile<st_freq_mem>;

template <> bool ConfigFile<st_config>::save(const st_config *cfg) {

    UINT bw;

#define CFG_VERSION(ID) WRITE_FIELD("version=%s", cfg->version)

#define CFG_BOOL(KEY, ID, MEMBER) WRITE_FIELD(KEY "=%d", cfg->MEMBER)
#define CFG_U8(KEY, ID, MEMBER) WRITE_FIELD(KEY "=%d", cfg->MEMBER)
#define CFG_I8(KEY, ID, MEMBER) WRITE_FIELD(KEY "=%d", cfg->MEMBER)
#define CFG_U16(KEY, ID, MEMBER) WRITE_FIELD(KEY "=%d", cfg->MEMBER)
#define CFG_I16(KEY, ID, MEMBER) WRITE_FIELD(KEY "=%d", cfg->MEMBER)
#define CFG_I32(KEY, ID, MEMBER) WRITE_FIELD(KEY "=%d", cfg->MEMBER)
#define CFG_U32(KEY, ID, MEMBER) WRITE_FIELD(KEY "=%lu", (unsigned long)cfg->MEMBER)
#define CFG_U32_L(KEY, ID, MEMBER) WRITE_FIELD(KEY "=%lu", (unsigned long)cfg->MEMBER)
#define CFG_FLOAT(KEY, ID, MEMBER) WRITE_FIELD(KEY "=%f", cfg->MEMBER)
#define CFG_ENUM(KEY, ID, MEMBER) WRITE_FIELD(KEY "=%d", (int)cfg->MEMBER)

#define CFG_VFO_ARRAY(ID)                                                                                                                                     \
    for (size_t i = 0; i < (sizeof(cfg->vfo) / sizeof(cfg->vfo[0])); ++i) {                                                                                    \
        WRITE_FIELD("vfo[%d].freq=%lu", (int)i, (unsigned long)cfg->vfo[i].freq);                                                                 \
        WRITE_FIELD("vfo[%d].step=%lu", (int)i, (unsigned long)cfg->vfo[i].step);                                                                 \
        WRITE_FIELD("vfo[%d].rit=%d", (int)i, cfg->vfo[i].rit);                                                                                                \
        WRITE_FIELD("vfo[%d].modulation=%d", (int)i, cfg->vfo[i].mode);                                                                                        \
    }

#define CFG_FFT_IQ_BLOBS(ID)                                                                                                                                 \
    write_bin("fft.iq_balance_meanZ=", (uint8_t *)cfg->fft.iq_balance_meanZ, sizeof(cfg->fft.iq_balance_meanZ));                                             \
    write_bin("fft.iq_balance_precZ=", reinterpret_cast<const uint8_t *>(cfg->fft.iq_balance_precZ), sizeof(cfg->fft.iq_balance_precZ))

#include "config_schema.def"

#undef CFG_FFT_IQ_BLOBS
#undef CFG_VFO_ARRAY

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
#undef CFG_VERSION

    return true;
}

template <> bool ConfigFile<st_config>::load(st_config *cfg) {

    // Robust, human-friendly loader:
    // - allows reordering and extra keys
    // - keeps defaults for missing keys
    // - handles the large FFT IQ blobs via streaming reads

    bool ok = true;
    bool saw_version = false;

    auto strip_newline = [](char *s) {
        if (!s) {
            return;
        }
        size_t n = std::strlen(s);
        while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
            s[n - 1] = 0;
            --n;
        }
    };

    auto ltrim_ws = [](char *s) -> char * {
        while (s && (*s == ' ' || *s == '\t')) {
            ++s;
        }
        return s;
    };

    auto rtrim_ws = [](char *s) {
        if (!s) {
            return;
        }
        size_t n = std::strlen(s);
        while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t')) {
            s[n - 1] = 0;
            --n;
        }
    };

    while (true) {
        const auto line_start = f_tell(file);
        if (f_gets(buf, sizeof(buf), file) == nullptr) {
            break;
        }

        strip_newline(buf);

        char *p = ltrim_ws(buf);
        if (*p == 0) {
            continue;
        }
        if (*p == '#' || *p == ';') {
            continue;
        }

        // Special: binary blobs are written as:
        //   fft.iq_balance_meanZ=\n<hexstream>\n
        if (std::strncmp(p, "fft.iq_balance_meanZ=", std::strlen("fft.iq_balance_meanZ=")) == 0) {
            (void)f_lseek(file, line_start);
            ok = ok && read_bin("fft.iq_balance_meanZ=", reinterpret_cast<uint8_t *>(cfg->fft.iq_balance_meanZ), sizeof(cfg->fft.iq_balance_meanZ));
            continue;
        }
        if (std::strncmp(p, "fft.iq_balance_precZ=", std::strlen("fft.iq_balance_precZ=")) == 0) {
            (void)f_lseek(file, line_start);
            ok = ok && read_bin("fft.iq_balance_precZ=", reinterpret_cast<uint8_t *>(cfg->fft.iq_balance_precZ), sizeof(cfg->fft.iq_balance_precZ));
            continue;
        }

        char *eq = std::strchr(p, '=');
        if (!eq) {
            continue;
        }

        *eq = 0;
        char *key = p;
        char *value = eq + 1;

        rtrim_ws(key);
        value = ltrim_ws(value);

        const bool applied = io::config_kv::apply_kv(key, value, cfg);
        if (applied && std::strcmp(key, "version") == 0) {
            saw_version = true;
        }
    }

    return ok && saw_version;
}
