//
// Created by Angel Dust on 01/11/2019.
//
#include "settings.h"

#include "dsp/dsp_config.h"
#include "fatfs/fatfs.h"
#include "ff.h"
#include "hw/stm32f4xx/eeprom.h"
#include "io/config/config_file.h"
#include "io/config/config_journal.h"
#include "main.h"
#include "hw/stm32.h"
#include "status.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_flash.h"
#include "types.h"
#include "ui/frequency_memory_ui.h"

#include "utils.hpp"

#include <cstring>

// Try to read the config from the SD Card
ConfigFile<> config_file;
// This is stored in flash (so no worry for BSS)
static const Config default_cfg{};

// Baseline for journal diffing (small; excludes FFT blobs).
static io::config_journal::st_config_journal_baseline last_journaled{};
static bool last_journaled_valid = false;

#if ENABLE_SD_CARD
static void compact_sd_config_if_needed(const Config &cfg) {

    // Keep this conservative: compaction implies a full snapshot rewrite.
    constexpr uint32_t COMPACT_THRESHOLD_BYTES = 2048;

    if (sdcard_info.status != sdcard_STATUS::Mounted) {
        return;
    }

    const uint32_t journal_size = io::config_journal::size_bytes("config.jrn");
    if (journal_size == 0 || journal_size <= COMPACT_THRESHOLD_BYTES) {
        return;
    }

    // 1) Rotate snapshot backups using renames (metadata-only).
    if (lock_sd_card(0, "cfg_compact_rotate")) {
        (void)f_unlink("config.bak2.cfg");
        (void)f_rename("config.bak1.cfg", "config.bak2.cfg");
        (void)f_unlink("config.bak1.cfg");
        (void)f_rename("config.cfg", "config.bak1.cfg");
        unlock_sd_card();
    }

    // 2) Write new snapshot to a temp file.
    ConfigFile<> snapshot_writer;
    const bool wrote_snapshot = snapshot_writer.save("config.tmp", &cfg);
    if (!wrote_snapshot) {
        return;
    }

    // 3) Promote temp snapshot and clear the journal.
    if (lock_sd_card(0, "cfg_compact_promote")) {
        (void)f_unlink("config.cfg");
        (void)f_rename("config.tmp", "config.cfg");
        (void)f_unlink("config.jrn");
        unlock_sd_card();
    }
}
#endif

uint8_t settings_read(Config *settings) {

    bool ok = false;

#if ENABLE_SD_CARD
    if (sdcard_info.status == sdcard_STATUS::Mounted) {

        // Avoid large stack temporaries: reuse caller-provided buffer.
        *settings = default_cfg;

        auto try_load_snapshot = [&](const char *filename) {
            // Ensure missing keys keep defaults.
            *settings = default_cfg;
            LOG("Loading %s\n", filename);

            if (!config_file.load(filename, settings)) {
                LOG("%s no found or failed loading\n", filename);
                return false;
            }
            if (memcmp(settings->version, CONFIG_VERSION, 3) != 0) {
                LOG("Config version in %s is %s. Expected %s. Saving it.\n", filename, settings->version, CONFIG_VERSION);
                std::strncpy(settings->version, CONFIG_VERSION, 4);
                config_file.save("config.cfg", settings);
            }
            return true;
        };

        ok = try_load_snapshot("config.cfg") || try_load_snapshot("config.bak1.cfg") || try_load_snapshot("config.bak2.cfg");

        if (ok) {
#if DEBUG
            LOG("settings_read: stack used pre-jrn: %u\n", (unsigned)stack_used_bytes_worst_case());
#endif
            // Journal is optional: replay whatever is available.
            (void)io::config_journal::replay("config.jrn", settings);

#if DEBUG
            LOG("settings_read: stack used post-jrn: %u\n", (unsigned)stack_used_bytes_worst_case());
#endif

            // Establish baseline for runtime journal diffing.
            io::config_journal::init_baseline(*settings, &last_journaled);
            last_journaled_valid = true;

            compact_sd_config_if_needed(*settings);

            return EE_OK;
        }
    }
#endif

    // Read config from flash
    *settings = default_cfg;

    LOG("Loading settings from FLASH memory\n");
    // Read 2x uint16_t (4 bytes) from flash; keep buffer sized accordingly.
    char version[4];
    uint8_t status = flash_read((uint16_t *)version, 2);

    if (status == EE_OK) {
        if (memcmp(version, &settings->version, 3) == 0) {
            status = flash_read((uint16_t *)settings, ceil((float)sizeof(Config) / (float)sizeof(uint16_t)));
        } else {
            LOG("Config version in FLASH is %s. Expected %s. Saving default.\n", version, settings->version);
            config_file.save("config.cfg", settings);
        }
    }

    io::config_journal::init_baseline(*settings, &last_journaled);
    last_journaled_valid = true;

    return status;
}

uint8_t settings_write(Config *settings) {

    // TODO: Make plugin-like configuration system so things like the dsp subsystem is not so coupled here
    settings->dsp = dsp::dsp_config;

#if ENABLE_SD_CARD
    if (sdcard_info.status == sdcard_STATUS::Mounted) {

        if (!last_journaled_valid) {
            io::config_journal::init_baseline(*settings, &last_journaled);
            last_journaled_valid = true;
        }

        bool wrote_any = false;
        const bool ok = io::config_journal::append_changes("config.jrn", *settings, &last_journaled, &wrote_any);

        if (!ok) {
            status::pop_alert(status::ERROR, "Error saving config journal in SD card. Fallback to Flash");
        } else {
            (void)wrote_any;
            return 0;
        }
    }
#endif

    // Fallback: store full settings in flash.
    io::config_journal::init_baseline(*settings, &last_journaled);
    last_journaled_valid = true;

    return flash_write((uint16_t *)settings, ceil((float)sizeof(Config) / (float)sizeof(uint16_t)));
}

// uint8_t settings_write(st_freq_mem *mem) {

//     bool ok = false;

// #if ENABLE_SD_CARD
//     if (sdcard_info.status == sdcard_STATUS::Mounted) {

//         ConfigFile<st_freq_mem> config_file;

//         ok = config_file.save("mem.db", mem);

//         if (!ok) {
//             status::pop_alert(status::ERROR, "Error saving memory in SD card");
//         }
//     }
// #else
//     return flash_write((uint16_t *)&config, ceil((float)sizeof(Config) / (float)sizeof(uint16_t)));
// #endif

//     return ok ? 0 : 1;
// }

// static void _settings_reset_to_defaults(Config *settings) {
//
//     memset(settings, 0, sizeof(Config));
//     //TODO: define default values
// }
