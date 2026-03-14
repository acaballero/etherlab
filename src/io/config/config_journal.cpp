// Created by ECA on 2026-03-14.

#include "config_journal.h"

#include "fatfs/fatfs.h"
#include "ff.h"
#include "config_kv_apply.h"
#include "types.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace io::config_journal {

static void strip_newline(char *s) {
    if (!s) {
        return;
    }

    size_t n = std::strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[n - 1] = 0;
        --n;
    }
}

static const char *skip_ws(const char *s) {
    while (s && (*s == ' ' || *s == '\t')) {
        ++s;
    }
    return s;
}

static bool journal_vprintf(FIL *file, const char *fmt, va_list ap) {
    if (!file || !fmt) {
        return false;
    }

    char line[160];
    int n = vsnprintf(line, sizeof(line), fmt, ap);
    if (n <= 0) {
        return false;
    }

    UINT bw = 0;
    return f_write(file, line, (UINT)std::strlen(line), &bw) == FR_OK;
}

static bool journal_printf(FIL *file, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    bool ok = journal_vprintf(file, fmt, ap);
    va_end(ap);
    return ok;
}

uint32_t size_bytes(const char *filename) {

    if (!filename || !lock_sd_card(0, "cfg_jrn_size")) {
        return 0;
    }

    FIL f;
    uint32_t size = 0;

    if (f_open(&f, filename, FA_READ) == FR_OK) {
        size = (uint32_t)f_size(&f);
        f_close(&f);
    }

    unlock_sd_card();
    return size;
}

bool replay(const char *filename, st_config *cfg, uint32_t *applied_lines) {

    if (applied_lines) {
        *applied_lines = 0;
    }

    if (!filename || !cfg) {
        return false;
    }

    if (!lock_sd_card(0, "cfg_jrn_replay")) {
        return false;
    }

    FIL f;
    FRESULT res = f_open(&f, filename, FA_READ);
    if (res != FR_OK) {
        unlock_sd_card();
        return true; // missing journal is OK
    }

    char line[256];
    while (f_gets(line, sizeof(line), &f) != nullptr) {

        strip_newline(line);
        const char *p = skip_ws(line);

        if (*p == 0) {
            continue;
        }
        if (*p == '#' || *p == ';') {
            continue;
        }

        const char *eq = std::strchr(p, '=');
        if (!eq) {
            continue;
        }

        char key[96];
        size_t klen = (size_t)(eq - p);
        if (klen == 0 || klen >= sizeof(key)) {
            continue;
        }

        std::memcpy(key, p, klen);
        key[klen] = 0;

        const char *val = eq + 1;

        if (io::config_kv::apply_kv(key, val, cfg)) {
            if (applied_lines) {
                ++(*applied_lines);
            }
        }
    }

    f_close(&f);
    unlock_sd_card();
    return true;
}

void init_baseline(const st_config &cfg, st_config_journal_baseline *baseline) {

    if (!baseline) {
        return;
    }

    for (size_t i = 0; i < st_config_journal_baseline::VFO_COUNT; ++i) {
        baseline->vfo[i] = cfg.vfo[i];
    }

#define CFG_VERSION(ID) ((void)0)
#define CFG_VFO_ARRAY(ID) ((void)0)
#define CFG_FFT_IQ_BLOBS(ID) ((void)0)

#define CFG_BOOL(KEY, ID, MEMBER) baseline->ID = cfg.MEMBER
#define CFG_U8(KEY, ID, MEMBER) baseline->ID = cfg.MEMBER
#define CFG_I8(KEY, ID, MEMBER) baseline->ID = cfg.MEMBER
#define CFG_U16(KEY, ID, MEMBER) baseline->ID = cfg.MEMBER
#define CFG_I16(KEY, ID, MEMBER) baseline->ID = cfg.MEMBER
#define CFG_I32(KEY, ID, MEMBER) baseline->ID = cfg.MEMBER
#define CFG_U32(KEY, ID, MEMBER) baseline->ID = cfg.MEMBER
#define CFG_U32_L(KEY, ID, MEMBER) baseline->ID = cfg.MEMBER
#define CFG_FLOAT(KEY, ID, MEMBER) baseline->ID = cfg.MEMBER
#define CFG_ENUM(KEY, ID, MEMBER) baseline->ID = cfg.MEMBER

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

#undef CFG_FFT_IQ_BLOBS
#undef CFG_VFO_ARRAY
#undef CFG_VERSION
}

bool append_changes(const char *filename, const st_config &current, st_config_journal_baseline *baseline, bool *wrote_any) {

    if (wrote_any) {
        *wrote_any = false;
    }

    if (!filename || !baseline) {
        return false;
    }

    // Lazily open the journal only if we actually need to write.
    bool opened = false;
    bool ok = true;
    FIL f;

    auto ensure_open = [&]() -> bool {
        if (opened) {
            return true;
        }
        if (!lock_sd_card(0, "cfg_jrn_append")) {
            return false;
        }
        if (f_open(&f, filename, FA_WRITE | FA_OPEN_ALWAYS) != FR_OK) {
            unlock_sd_card();
            return false;
        }
        if (f_lseek(&f, f_size(&f)) != FR_OK) {
            f_close(&f);
            unlock_sd_card();
            return false;
        }
        opened = true;
        return true;
    };

    const st_config &cfg = current;

#define CFG_VERSION(ID) ((void)0)
#define CFG_FFT_IQ_BLOBS(ID) ((void)0)

#define JRN_MARK_WRITTEN()                                                                                                                                     \
    do {                                                                                                                                                       \
        if (wrote_any) {                                                                                                                                      \
            *wrote_any = true;                                                                                                                                \
        }                                                                                                                                                      \
    } while (0)

#define JRN_UPDATE(CURRENT_VALUE, BASELINE_LVALUE, PRINT_STMT)                                                                                                 \
    do {                                                                                                                                                       \
        if (ok && ((CURRENT_VALUE) != (BASELINE_LVALUE))) {                                                                                                   \
            ok = ensure_open();                                                                                                                                \
            if (ok) {                                                                                                                                         \
                ok = (PRINT_STMT);                                                                                                                             \
            }                                                                                                                                                  \
            if (ok) {                                                                                                                                         \
                (BASELINE_LVALUE) = (CURRENT_VALUE);                                                                                                           \
                JRN_MARK_WRITTEN();                                                                                                                            \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    } while (0)

#define CFG_BOOL(KEY, ID, MEMBER) JRN_UPDATE(cfg.MEMBER, baseline->ID, journal_printf(&f, KEY "=%d\n", cfg.MEMBER ? 1 : 0))
#define CFG_U8(KEY, ID, MEMBER) JRN_UPDATE(cfg.MEMBER, baseline->ID, journal_printf(&f, KEY "=%u\n", (unsigned)cfg.MEMBER))
#define CFG_I8(KEY, ID, MEMBER) JRN_UPDATE(cfg.MEMBER, baseline->ID, journal_printf(&f, KEY "=%d\n", (int)cfg.MEMBER))
#define CFG_U16(KEY, ID, MEMBER) JRN_UPDATE(cfg.MEMBER, baseline->ID, journal_printf(&f, KEY "=%u\n", (unsigned)cfg.MEMBER))
#define CFG_I16(KEY, ID, MEMBER) JRN_UPDATE(cfg.MEMBER, baseline->ID, journal_printf(&f, KEY "=%d\n", (int)cfg.MEMBER))
#define CFG_I32(KEY, ID, MEMBER) JRN_UPDATE(cfg.MEMBER, baseline->ID, journal_printf(&f, KEY "=%ld\n", (long)cfg.MEMBER))
#define CFG_U32(KEY, ID, MEMBER) JRN_UPDATE(cfg.MEMBER, baseline->ID, journal_printf(&f, KEY "=%lu\n", (unsigned long)cfg.MEMBER))
#define CFG_U32_L(KEY, ID, MEMBER) JRN_UPDATE(cfg.MEMBER, baseline->ID, journal_printf(&f, KEY "=%lu\n", (unsigned long)cfg.MEMBER))
#define CFG_FLOAT(KEY, ID, MEMBER) JRN_UPDATE(cfg.MEMBER, baseline->ID, journal_printf(&f, KEY "=%f\n", cfg.MEMBER))
#define CFG_ENUM(KEY, ID, MEMBER) JRN_UPDATE(cfg.MEMBER, baseline->ID, journal_printf(&f, KEY "=%d\n", (int)cfg.MEMBER))

#define CFG_VFO_ARRAY(ID)                                                                                                                                     \
    for (size_t i = 0; ok && i < st_config_journal_baseline::VFO_COUNT; ++i) {                                                                                \
        JRN_UPDATE(cfg.vfo[i].freq, baseline->vfo[i].freq, journal_printf(&f, "vfo[%u].freq=%lu\n", (unsigned)i, (unsigned long)cfg.vfo[i].freq));          \
        JRN_UPDATE(cfg.vfo[i].step, baseline->vfo[i].step, journal_printf(&f, "vfo[%u].step=%lu\n", (unsigned)i, (unsigned long)cfg.vfo[i].step));          \
        JRN_UPDATE(cfg.vfo[i].rit, baseline->vfo[i].rit, journal_printf(&f, "vfo[%u].rit=%ld\n", (unsigned)i, (long)cfg.vfo[i].rit));                        \
        JRN_UPDATE(cfg.vfo[i].mode, baseline->vfo[i].mode, journal_printf(&f, "vfo[%u].modulation=%d\n", (unsigned)i, (int)cfg.vfo[i].mode));               \
    }

#include "config_schema.def"

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

#undef JRN_UPDATE
#undef JRN_MARK_WRITTEN

#undef CFG_FFT_IQ_BLOBS
#undef CFG_VERSION

    if (opened) {
        f_close(&f);
        unlock_sd_card();
    }

    return ok;
}

} // namespace io::config_journal
