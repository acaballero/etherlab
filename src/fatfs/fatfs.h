
#ifndef __fatfs_H
#define __fatfs_H

#include "../../lib/FatFs/ff.h"
#include "spi_diskio.h" /* defines USER_Driver as external */
#include "../lib/Signal/Signal.h"
#include "os/periodic_task.h"

namespace sdcard {
extern os::periodic_task task;
}

#ifdef __cplusplus
extern "C" {
#endif

/* USER CODE BEGIN Includes */
#define SDCARD_LOOP_PERIOD_MS 2000
#define FN_SIZE _MAX_LFN // _MAX_LFN is defined int FatFS library
#define PATH_SIZE FN_SIZE * 2

extern uint64_t sdcard_last_check_ms;

enum sdcard_STATUS { IOError = -3, MountError = -2, ConnectError = -1, NotPresent = 0, Present = 1, Mounted = 2, MassStorageDeviceActive = 3 };

struct sdcard_st_info {
    sdcard_STATUS status = NotPresent;
    uint32_t sectors = 0;
    uint32_t free_kb = 0;

    bool operator==(const sdcard_st_info &st) const {
        return status == st.status && sectors == st.sectors && free_kb == st.free_kb;
    }
};

/* USER CODE END Includes */

extern char USERPath[4];    /* USER logical drive path */
extern FATFS FatFS;         /* File system object for USER logical drive */
extern FIL FatFSFileHandle; /* Shared file object */
extern sdcard_st_info sdcard_info;
extern Signal sdcard_signal;
extern volatile bool sd_card_locked;
extern volatile uint8_t usb_msc_active; /* Defined in usb.cpp */
void sdcard_loop(void);
void sdcard_init();
void test_sd_card();
bool lock_sd_card();
bool unlock_sd_card();

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */
#ifdef __cplusplus
}
#endif
#endif /*__fatfs_H */
