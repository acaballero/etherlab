//
// Created by Angel Dust on 01/11/2019.
//
#include "settings.h"
#include "dsp/dsp_config.h"
#include "fatfs/fatfs.h"
#include "hw/stm32f4xx/eeprom.h"
#include "main.h"
#include "hw/stm32.h"
#include "status.h"
#include "stm32f4xx_hal_flash.h"
#include "io/config_file.h"
#include "usbd_cdc_if.h"
#include <cstring>

//_Static_assert(sizeof(Config) < DATA_EEPROM_SIZE_BYTES * NFLASHPAGES, "EEPROM struct too large!");

// static void _settings_reset_to_defaults(Config *settings);
/*
uint8_t settings_read(Config *settings) {

    // If the version of the settings stored in flash is different from the version of the Config struct, we don't read them
    char version[3];

    // memcpy(version, (uint32_t *) (DATA_EEPROM_START_ADDR + sizeof(Config) - 4), 3); // version is the last member of the struct
    memcpy(version, (uint32_t *)(DATA_EEPROM_START_ADDR), 3); // version is the first member of the struct

    uint8_t status = 0;

    if (memcmp(version, &settings->version, 3) == 0) {

        // copy data from EEPROM to RAM
        memcpy(settings, (uint32_t *)DATA_EEPROM_START_ADDR, sizeof(Config));

        // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);

        // CRC_ResetDR();

        // uint32_t computed_crc = CRC_CalcBlockCRC(
        //         (uint32_t *)GLOBAL_settings_ptr,
        //         (sizeof(Config)-sizeof(uint32_t))/sizeof(uint32_t)//size minus the crc32 at the end, IN WORDS
        //);

        // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, DISABLE);

        // if (computed_crc != GLOBAL_settings_ptr->crc32){
        //      _settings_reset_to_defaults();
        // }

        status = 1;
    }

    return status;
}
*/

uint8_t settings_read(Config *settings) {

    // If the version of the settings stored is different from the version of the Config struct, we don't read them

    bool ok = false;
    char version[3];

#if ENABLE_SD_CARD
    if (sdcard_info.status == sdcard_STATUS::Mounted) {

        // Try to read the config from the SD Card
        ConfigFile config_file;

        memcpy(version, settings->version, 3);

        ok = config_file.load("config.cfg", *settings);

        if (ok) {
            ok = (memcmp(version, &settings->version, 3) == 0);
        }
    }
#endif
    if (!ok) {

        // Read config from flash

        uint8_t status = flash_read((uint16_t *)version, 2);

        if (status == EE_OK) {
            if (memcmp(version, &settings->version, 3) == 0) {
                status = flash_read((uint16_t *)settings, ceil((float)sizeof(Config) / (float)sizeof(uint16_t)));
            }
        }

        return status;
    } else {
        return true;
    }
}

uint8_t settings_write(Config *settings) {

    bool ok = false;

    // TODO: Make plugin-like configuration system so things like the dsp subsystem is not so coupled here
    config.dsp = dsp::dsp_config;

#if ENABLE_SD_CARD

    if (sdcard_info.status == sdcard_STATUS::Mounted) {

        // Try to read the config from the SD Card
        ConfigFile config_file;

        ok = config_file.save("config.cfg", *settings);

        if (!ok) {
            status::handleError(status::ST_ERROR, "Error saving config in SD card. Fallback to Flash");
        }
    }

#endif

    if (!ok) {
        return flash_write((uint16_t *)settings, ceil((float)sizeof(Config) / (float)sizeof(uint16_t)));
    } else {
        return 0;
    }
}

// static void _settings_reset_to_defaults(Config *settings) {
//
//     memset(settings, 0, sizeof(Config));
//     //TODO: define default values
// }
