
#ifndef __SD_DISKIO_H
#define __SD_DISKIO_H

#include "bsp_driver_sd.h"
#include "../../lib/FatFs/ff_gen_drv.h"

extern Diskio_drvTypeDef SD_Driver;

#ifdef __cplusplus
extern "C" {
#endif

DRESULT SD_read_dma(BYTE lun, BYTE *buff, DWORD sector, UINT count);
DRESULT SD_write_dma(BYTE lun, const BYTE *buff, DWORD sector, UINT count);

#ifdef __cplusplus
}
#endif

#endif
