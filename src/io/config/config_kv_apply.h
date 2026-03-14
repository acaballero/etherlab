// Created by ECA on 2026-03-14.
//
// Shared key/value application for st_config.
// Used by both snapshot parsing and the config journal.

#ifndef TRX_CONFIG_KV_APPLY_H
#define TRX_CONFIG_KV_APPLY_H

#include "config.h"

namespace io::config_kv {

// Applies a single key/value pair onto an existing config.
// Returns true if the key was recognized and applied.
bool apply_kv(const char *key, const char *value, st_config *cfg);

} // namespace io::config_kv

#endif // TRX_CONFIG_KV_APPLY_H
