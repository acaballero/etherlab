// Created by ECA on 2026-03-14.
//
// Append-only journal for configuration changes.
//
// Goal: minimize runtime SD-card writes by appending only changed key/value pairs.
// The journal is human-editable: one setting per line in the form "key=value".

#ifndef TRX_CONFIG_JOURNAL_H
#define TRX_CONFIG_JOURNAL_H

#include "config.h"
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace io::config_journal {

// Small baseline snapshot used to detect which fields have changed since the
// last successful journal append.
//
// IMPORTANT: This intentionally excludes large binary fields (FFT IQ blobs).
struct st_config_journal_baseline {
    // NOTE: keep this in sync with `st_config::vfo` count.
    static constexpr size_t VFO_COUNT = sizeof(((st_config *)nullptr)->vfo) / sizeof(((st_config *)nullptr)->vfo[0]);

    // Baseline VFO array.
    st_vfo_config vfo[VFO_COUNT]{};

    // Baseline scalar fields (generated from schema)
#define CFG_VERSION(ID) /* no baseline */
#define CFG_VFO_ARRAY(ID) /* handled above */
#define CFG_FFT_IQ_BLOBS(ID) /* excluded */

#define CFG_BOOL(KEY, ID, MEMBER) std::decay_t<decltype(st_config{}.MEMBER)> ID{};
#define CFG_U8(KEY, ID, MEMBER) std::decay_t<decltype(st_config{}.MEMBER)> ID{};
#define CFG_I8(KEY, ID, MEMBER) std::decay_t<decltype(st_config{}.MEMBER)> ID{};
#define CFG_U16(KEY, ID, MEMBER) std::decay_t<decltype(st_config{}.MEMBER)> ID{};
#define CFG_I16(KEY, ID, MEMBER) std::decay_t<decltype(st_config{}.MEMBER)> ID{};
#define CFG_I32(KEY, ID, MEMBER) std::decay_t<decltype(st_config{}.MEMBER)> ID{};
#define CFG_U32(KEY, ID, MEMBER) std::decay_t<decltype(st_config{}.MEMBER)> ID{};
#define CFG_U32_L(KEY, ID, MEMBER) std::decay_t<decltype(st_config{}.MEMBER)> ID{};
#define CFG_FLOAT(KEY, ID, MEMBER) std::decay_t<decltype(st_config{}.MEMBER)> ID{};
#define CFG_ENUM(KEY, ID, MEMBER) std::decay_t<decltype(st_config{}.MEMBER)> ID{};

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
};

// Initialize a baseline from a fully-populated config.
void init_baseline(const st_config &cfg, st_config_journal_baseline *baseline);

// Replays the journal onto an already-loaded config snapshot.
// Returns true on success or if the journal does not exist.
bool replay(const char *filename, st_config *cfg, uint32_t *applied_lines = nullptr);

// Appends only the scalar changes from `current` relative to `baseline`.
// On success, updates `*baseline` to match `current` for the journaled fields.
// Returns true on success (including when no changes are detected).
bool append_changes(const char *filename, const st_config &current, st_config_journal_baseline *baseline, bool *wrote_any = nullptr);

// Returns the journal size in bytes, or 0 if missing/error.
uint32_t size_bytes(const char *filename);

} // namespace io::config_journal

#endif // TRX_CONFIG_JOURNAL_H
