/**
 ******************************************************************************
 * @file   fatfs.c
 * @brief  Code for fatfs applications
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */

#include "fatfs.h"
#include "hw/stm32f4xx/usb.h"

#include "status.h"
#include "hw/stm32_hal.h"
#include "../../lib/FatFs/ff.h"
#include "../../lib/FatFs/diskio.h"
#include "usb/usbd_msc.h"
#include <stdio.h>

extern Diskio_drvTypeDef SD_CARD_DRIVER; // Defined in the parent project
extern USBD_HandleTypeDef hUsbDeviceHS;  // Defined in usbd_msc.h

char USERPath[4];    /* USER logical drive path */
FATFS FatFS;         /* File system object for USER logical drive */
FIL FatFSFileHandle; /* File object for USER */

/* USER CODE BEGIN Variables */

sdcard_st_info sdcard_info;
uint64_t sdcard_last_check_ms;
Signal sdcard_signal;
volatile bool sd_card_locked = false;

void sdcard_loop();

namespace sdcard {
os::periodic_task task(2000, sdcard_loop);
}

/*
 * Poll to update the status of the SD card
 */
void sdcard_loop() {
    uint64_t t = HAL_GetTick();

    if (usb_msc_active) {
        if (hUsbDeviceHS.dev_state ==
            USBD_STATE_CONFIGURED) { // Some hosts (almost all) don't cause a MSC_DeInit when the USB is detacched or unplugged so we also check the dev_state

            // Don't touch the SD card when the host is controlling it as a MSC (mass storage device)
            sdcard_info.status = MassStorageDeviceActive;
            sdcard_signal.emit(&sdcard_info);
        } else {
            if (hUsbDeviceHS.dev_old_state == USBD_STATE_CONFIGURED && hUsbDeviceHS.dev_state != USBD_STATE_CONFIGURED) {
                sdcard_info.status = Present;
                unlock_sd_card(); // TODO: This has to be unlocked by the one who locked it (usb MSC initialization in usb.cpp)
                init_USB_CDC();
                sdcard_signal.emit(&sdcard_info);
            }
        }
    }

    if (!usb_msc_active && t - sdcard_last_check_ms > SDCARD_LOOP_PERIOD_MS) {
        sdcard_last_check_ms = t;
        sdcard_init();
    }
}

bool try_lock_sd_card() {
    bool b;
    if (sd_card_locked || usb_msc_active) {
        b = false;
    } else {
        sd_card_locked = true;
        if (SDIO_GetPowerState(SDIO_HANDLE.Instance) == 0) {
            SDIO_PowerState_ON(SDIO_HANDLE.Instance);
        }
        b = true;
    }

    //    LOG("SDcard locked: %d, state: %d\n", b, sd_card_locked);
    return b;
}

bool lock_sd_card(uint32_t timeout_ms) {
    // TODO: Save who locked it and prevent other client to unlock
    uint32_t start = HAL_GetTick();
    while (!try_lock_sd_card()) {
        if ((HAL_GetTick() - start) > timeout_ms) {
            return false; // timeout
        }
    }
    return true;
}

bool unlock_sd_card() {
    bool b;
    if (sd_card_locked && sdcard_info.status != MassStorageDeviceActive) { // note: prevent someone powering the sd device off while MSD is on
        SDIO_PowerState_OFF(SDIO_HANDLE.Instance);
        sd_card_locked = false;
        b = true;
    } else {
        b = false;
    }

    // LOG("SDcard UNlocked: %d, state: %d\n", b, sd_card_locked);
    return b;
}

FRESULT check_sd_card_health(void) {
    // Level 1: Quick disk status check
    if (disk_status(0) != RES_OK) {
        return FR_DISK_ERR;
    }

    DIR dir;
    return f_opendir(&dir, ""); // Check root directory
}
/* USER CODE END Variables */

void sdcard_init(void) {

    /* Link the USER driver */
    // uint8_t retUSER;

    if (lock_sd_card()) {

        sdcard_st_info new_status;
        // TODO: check for presence
        new_status.status = NotPresent;
        FRESULT fres = FR_OK;
        DSTATUS stat = RES_OK;
        bool error = false;

        if (sdcard_info.status == NotPresent) {
            FATFS_LinkDriver(&SD_CARD_DRIVER, USERPath);
        } else if (sdcard_info.status == MountError) {
            stat = SD_CARD_DRIVER.disk_initialize(0);
            memset(&FatFS, 0, sizeof(FatFS)); // Clear the status
        }

        if (stat == RES_OK) {
            stat = check_sd_card_health();
            if (stat != FR_OK) {
                // Try to remount
                fres = f_mount(&FatFS, "/", 1); // 1=mount now
                error = fres != FR_OK;
            }
        } else {
            error = true;
        }

        if (error) {

            new_status.status = MountError;
            // handleError(status::ST_ERROR, "f_mount error");

        } else {
            // Let's get some statistics from the SD card
            DWORD free_clusters, free_sectors, total_sectors;

            FATFS *getFreeFs;

            fres = f_getfree("", &free_clusters, &getFreeFs);

            if (fres != FR_OK) {
                handleError(status::ST_ERROR, "f_getfree error");
                new_status.status = IOError;
            } else {

                // Formula comes from ChaN's documentation
                total_sectors = (getFreeFs->n_fatent - 2) * getFreeFs->csize;
                free_sectors = free_clusters * getFreeFs->csize;

                new_status.free_kb = free_sectors / 2;
                new_status.sectors = total_sectors / 2;
                new_status.status = Mounted;

                /* LOG("SD card stats:\r\n%10lu KiB total drive space.\r\n%10lu KiB available.\r\n",
                            sdcard_info.sectors,
                            sdcard_info.free_kb);*/
            }
        }

        if (!(new_status == sdcard_info)) {
            // Inform the listeners
            sdcard_info = new_status;
            sdcard_signal.emit(&sdcard_info);
        }

        unlock_sd_card();
    }
}

/**
 * @brief  Gets Time from RTC
 * @param  None
 * @retval Time in DWORD
 */
DWORD get_fattime(void) {
    /* USER CODE BEGIN get_fattime */
    return 0;
    /* USER CODE END get_fattime */
}

#if DEBUG_SD_CARD

/* USER CODE BEGIN Application */

void test_sd_card_work() {
    lock_sd_card();

    FIL fil;               // File handle
    volatile FRESULT fres; // Result after operations

    // Now let's try and write a file "write.txt"
    fres = f_open(&fil, "write.txt", FA_WRITE | FA_OPEN_ALWAYS | FA_CREATE_ALWAYS);
    if (fres == FR_OK) {
        // printf("Opened 'write.txt' for writing\r\n");
    } else {
        printf("f_open error (%i)\r\n", fres);
    }

    // Write benchmark

    uint64_t elapsed;
    uint32_t bytesToWrite = 5000000;
    uint32_t totalBytes = 0;
    UINT bytesWrote;
    uint16_t bufsize = 64;
    uint8_t buf[64];
    char *p;

    memset((char *)buf, 12, bufsize);
    fifo.reset();
    uint64_t m = HAL_GetTick();

    // Write some bytes to force partial sector writes and test the performance overhead
    fres = f_write(&fil, buf, bufsize, &bytesWrote);

    while (totalBytes < bytesToWrite && fres == FR_OK) {

        fifo.writeBlock((char *)buf, bufsize);

        uint16_t av = fifo.available(&p);
        if (av > DSP_FIFO_BLOCK_BYTES) {
            fres = f_write(&fil, p, DSP_FIFO_BLOCK_BYTES, &bytesWrote);
            totalBytes += bytesWrote;
            fifo.consume(DSP_FIFO_BLOCK_BYTES, &p);
        }
    }

    elapsed = HAL_GetTick() - m;

    if (fres == FR_OK) {
        printf("Wrote %lu bytes in %lu ms: %lu Kb/s\r\n", totalBytes, (uint32_t)(elapsed), (uint32_t)(totalBytes / (elapsed)));
    } else {
        printf("f_write error\r\n");
    }

    fifo.reset();

    fres = f_close(&fil);

    // Now let's try to open file
    fres = f_open(&fil, "write.txt", FA_READ);
    if (fres != FR_OK) {
        printf("f_open error\r\n");
    }

    printf("Opened 'write.txt' for reading!\r\n");

    // We can either use f_read OR f_gets to get data out of files
    // f_gets is a wrapper on f_read that does some string formatting for us

    totalBytes = 0;
    m = HAL_GetTick();
    while (totalBytes < bytesToWrite && fres == FR_OK) {

        uint32_t free = fifo.free(&p);

        if (free >= DSP_FIFO_BLOCK_BYTES) {

            f_read(&fil, p, DSP_FIFO_BLOCK_BYTES, &bytesWrote);
            fifo.feed(DSP_FIFO_BLOCK_BYTES);
        }

        uint32_t av = fifo.available(&p);

        if (av >= bufsize) {
            memcpy((char *)buf, p, bufsize);
            fifo.consume(bufsize, &p);
            totalBytes += bufsize;
        }
    }
    elapsed = HAL_GetTick() - m;
    f_sync(&fil);
    if (fres == FR_OK) {
        printf("Read %lu bytes in %lu ms: %lu Kb/s\r\n", totalBytes, (uint32_t)(elapsed), (uint32_t)(totalBytes / (elapsed)));
    } else {
        printf("f_read error\r\n");
    }

    //    TCHAR* rres = f_gets((TCHAR*)readBuf, 30, &fil);
    //    if(rres != 0) {
    //        printf("Read string from 'test.txt' contents: %s\r\n", readBuf);
    //    } else {
    //        printf("f_gets error (%i)\r\n", fres);
    //    }

    // Be a tidy kiwi - don't forget to close your file!
    f_close(&fil);

    // We're done, so de-mount the drive
    //  f_mount(NULL, "", 0);
    unlock_sd_card();
}

void test_sd_card() {
    for (int i = 0; i < 4; i++) {
        test_sd_card_work();
    }
}

#endif

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
