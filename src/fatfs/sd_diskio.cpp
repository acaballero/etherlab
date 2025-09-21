
#include "hw/stm32_hal.h" /* Provide the low-level HAL functions */
#include "../../lib/FatFs/ff_gen_drv.h"
#include "sd_diskio.h"
#include "status.h"
#include "fatfs.h"

#define SD_TIMEOUT 2 * 1000

#define SD_DEFAULT_BLOCK_SIZE 512

/*
 * Some DMA requires 4-Byte aligned address buffer to correctly read/write data,
 * in FatFs some accesses aren't thus we need a 4-byte aligned scratch buffer to correctly
 * transfer data
 */
/* USER CODE BEGIN enableScratchBuffer */
/* #define ENABLE_SCRATCH_BUFFER */
/* USER CODE END enableScratchBuffer */

/* Private variables ---------------------------------------------------------*/
#if defined(ENABLE_SCRATCH_BUFFER)
#if defined(ENABLE_SD_DMA_CACHE_MAINTENANCE)
ALIGN_32BYTES(static uint8_t scratch[BLOCKSIZE]); // 32-Byte aligned for cache maintenance
#else
__ALIGN_BEGIN static uint8_t scratch[BLOCKSIZE] __ALIGN_END;
#endif
#endif
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

static volatile UINT WriteStatus = 0, ReadStatus = 0;

/* Private function prototypes -----------------------------------------------*/
static DSTATUS SD_CheckStatus(BYTE lun);

DSTATUS SD_initialize(BYTE);

DSTATUS SD_status(BYTE);

DRESULT SD_read(BYTE, BYTE *, DWORD, UINT);

DRESULT SD_read_dma(BYTE, BYTE *, DWORD, UINT);

#if _USE_WRITE == 1

DRESULT SD_write(BYTE, const BYTE *, DWORD, UINT);

DRESULT SD_write_dma(BYTE, const BYTE *, DWORD, UINT);

#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1

DRESULT SD_ioctl(BYTE, BYTE, void *);

#endif /* _USE_IOCTL == 1 */

Diskio_drvTypeDef SD_Driver = {
    SD_initialize, SD_status, SD_read,
#if _USE_WRITE == 1
    SD_write,
#endif /* _USE_WRITE == 1 */

#if _USE_IOCTL == 1
    SD_ioctl,
#endif /* _USE_IOCTL == 1 */
};

Diskio_drvTypeDef SD_Driver_DMA = {
    SD_initialize, SD_status, SD_read_dma,
#if _USE_WRITE == 1
    SD_write_dma,
#endif /* _USE_WRITE == 1 */

#if _USE_IOCTL == 1
    SD_ioctl,
#endif /* _USE_IOCTL == 1 */
};

static int SD_CheckStatusWithTimeout(uint32_t timeout) {
    uint32_t timer = HAL_GetTick();
    /* block until SDIO IP is ready again or a timeout occur */
    while (HAL_GetTick() - timer < timeout) {
        if (BSP_SD_GetCardState() == SD_TRANSFER_OK) {
            return 0;
        }
    }

    return -1;
}

static DSTATUS SD_CheckStatus(BYTE lun) {
    Stat = STA_NOINIT;

    if (BSP_SD_GetCardState() == MSD_OK) {
        Stat &= ~STA_NOINIT;
    }

    return Stat;
}

/**
 * @brief  Initializes a Drive
 * @param  lun : not used
 * @retval DSTATUS: Operation status
 */
DSTATUS SD_initialize(BYTE lun) {

#if !defined(DISABLE_SD_INIT)

    if (BSP_SD_Init() == MSD_OK) {
        Stat = SD_CheckStatus(lun);
    }

#else

    if (sdcard_info.status == MountError) {
        if (BSP_SD_Init() == MSD_OK) {
            Stat = SD_CheckStatus(lun);
        } else {
            Stat = RES_ERROR;
        }
    } else {
        Stat = SD_CheckStatus(lun);
    }

#endif

    return Stat;
}

/**
 * @brief  Gets Disk Status
 * @param  lun : not used
 * @retval DSTATUS: Operation status
 */
DSTATUS SD_status(BYTE lun) {
    return SD_CheckStatus(lun);
}

/**
 * @brief  Reads Sector(s)
 * @param  lun : not used
 * @param  *buff: Data buffer to store read data
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to read (1..128)
 * @retval DRESULT: Operation result
 */

DRESULT SD_read(BYTE lun, BYTE *buff, DWORD sector, UINT count) {
    DRESULT res = RES_ERROR;
    uint32_t timeout;
#if defined(ENABLE_SCRATCH_BUFFER)
    uint8_t ret;
#endif

    /*
     * ensure the SDCard is ready for a new operation
     */

    if (SD_CheckStatusWithTimeout(SD_TIMEOUT) < 0) {
        return res;
    }

#if defined(ENABLE_SCRATCH_BUFFER)
    if (!((uint32_t)buff & 0x3)) {
#endif
        if (BSP_SD_ReadBlocks((uint32_t *)buff, (uint32_t)(sector), count, SD_TIMEOUT) == MSD_OK) {

            timeout = HAL_GetTick();
            while ((HAL_GetTick() - timeout) < SD_TIMEOUT) {
                if (BSP_SD_GetCardState() == SD_TRANSFER_OK) {
                    res = RES_OK;
                    break;
                }
            }
        }

#if defined(ENABLE_SCRATCH_BUFFER)
    } else {
        /* Slow path, fetch each sector a part and memcpy to destination buffer */
        int i;

        for (i = 0; i < count; i++) {
            ret = BSP_SD_ReadBlocks((uint32_t *)scratch, (uint32_t)sector++, 1, SD_TIMEOUT);
            if (ret == MSD_OK) {
                memcpy(buff, scratch, BLOCKSIZE);
                buff += BLOCKSIZE;
            } else {
                break;
            }
        }

        if ((i == count) && (ret == MSD_OK))
            res = RES_OK;
    }
#endif

    return res;
}

/**
 * @brief  Reads Sector(s)
 * @param  lun : not used
 * @param  *buff: Data buffer to store read data
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to read (1..128)
 * @retval DRESULT: Operation result
 */

DRESULT SD_read_dma(BYTE lun, BYTE *buff, DWORD sector, UINT count) {
    DRESULT res = RES_ERROR;
    uint32_t timeout;
#if defined(ENABLE_SCRATCH_BUFFER)
    uint8_t ret;
#endif
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
    uint32_t alignedAddr;
#endif

    /*
     * ensure the SDCard is ready for a new operation
     */

    if (SD_CheckStatusWithTimeout(SD_TIMEOUT) < 0) {
        return res;
    }

#if defined(ENABLE_SCRATCH_BUFFER)
    if (!((uint32_t)buff & 0x3)) {
#endif
        // FIXME:: Have to disable tasks timer interrupt. Otherwise, this DMA operation timeouts "ramdomly" Fix it with proper priority handling.
        // Also, this assumes TIM14 as the one it has to disable
        NVIC_DisableIRQ(TIM8_TRG_COM_TIM14_IRQn);
        if (BSP_SD_ReadBlocks_DMA((uint32_t *)buff, (uint32_t)(sector), count) == MSD_OK) {
            ReadStatus = 0;
            /* Wait that the reading process is completed or a timeout occurs */
            timeout = HAL_GetTick();
            while ((ReadStatus == 0) && ((HAL_GetTick() - timeout) < SD_TIMEOUT)) {
            }
            /* in case of a timeout return error */
            if (ReadStatus == 0) {
                res = RES_ERROR;
            } else {
                ReadStatus = 0;
                timeout = HAL_GetTick();

                while ((HAL_GetTick() - timeout) < SD_TIMEOUT) {
                    if (BSP_SD_GetCardState() == SD_TRANSFER_OK) {
                        res = RES_OK;
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
                        /*
                        the SCB_InvalidateDCache_by_Addr() requires a 32-Byte aligned address,
                        adjust the address and the D-Cache size to invalidate accordingly.
                        */
                        alignedAddr = (uint32_t)buff & ~0x1F;
                        SCB_InvalidateDCache_by_Addr((uint32_t *)alignedAddr, count * BLOCKSIZE + ((uint32_t)buff - alignedAddr));
#endif
                        break;
                    }
                }
            }
        }
        NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);
#if defined(ENABLE_SCRATCH_BUFFER)
    } else {
        /* Slow path, fetch each sector a part and memcpy to destination buffer */
        int i;

        for (i = 0; i < count; i++) {
            ret = BSP_SD_ReadBlocks_DMA((uint32_t *)scratch, (uint32_t)sector++, 1);
            if (ret == MSD_OK) {
                /* wait until the read is successful or a timeout occurs */

                timeout = HAL_GetTick();
                while ((ReadStatus == 0) && ((HAL_GetTick() - timeout) < SD_TIMEOUT)) {
                }
                if (ReadStatus == 0) {
                    res = RES_ERROR;
                    break;
                }
                ReadStatus = 0;

#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
                /*
                 *
                 * invalidate the scratch buffer before the next read to get the actual data instead of the cached one
                 */
                SCB_InvalidateDCache_by_Addr((uint32_t *)scratch, BLOCKSIZE);
#endif
                memcpy(buff, scratch, BLOCKSIZE);
                buff += BLOCKSIZE;
            } else {
                break;
            }
        }

        if ((i == count) && (ret == MSD_OK))
            res = RES_OK;
    }
#endif

    return res;
}

/**
 * @brief  Writes Sector(s)
 * @param  lun : not used
 * @param  *buff: Data to be written
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to write (1..128)
 * @retval DRESULT: Operation result
 */
#if _USE_WRITE == 1

DRESULT SD_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count) {
    DRESULT res = RES_ERROR;
    uint32_t timeout;
#if defined(ENABLE_SCRATCH_BUFFER)
    uint8_t ret;
    int i;
#endif

    if (SD_CheckStatusWithTimeout(SD_TIMEOUT) < 0) {
        return res;
    }

#if defined(ENABLE_SCRATCH_BUFFER)
    if (!((uint32_t)buff & 0x3)) {
#endif

        if (BSP_SD_WriteBlocks((uint32_t *)buff, (uint32_t)(sector), count, SD_TIMEOUT) == MSD_OK) {
            /* Wait that writing process is completed or a timeout occurs */

            timeout = HAL_GetTick();
            while ((HAL_GetTick() - timeout) < SD_TIMEOUT) {
                if (BSP_SD_GetCardState() == SD_TRANSFER_OK) {
                    res = RES_OK;
                    break;
                }
            }
        }

#if defined(ENABLE_SCRATCH_BUFFER)
    } else {
        /* Slow path, fetch each sector a part and memcpy to destination buffer */

        for (i = 0; i < count; i++) {
            WriteStatus = 0;

            memcpy((void *)scratch, (void *)buff, BLOCKSIZE);
            buff += BLOCKSIZE;

            ret = BSP_SD_WriteBlocks((uint32_t *)scratch, (uint32_t)sector++, 1, SD_TIMEOUT);
            if (ret != MSD_OK) {
                break;
            }
        }
        if ((i == count) && (ret == MSD_OK))
            res = RES_OK;
    }
#endif
    return res;
}

DRESULT SD_write_dma(BYTE lun, const BYTE *buff, DWORD sector, UINT count) {
    DRESULT res = RES_ERROR;
    uint32_t timeout;
#if defined(ENABLE_SCRATCH_BUFFER)
    uint8_t ret;
    int i;
#endif

    WriteStatus = 0;
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
    uint32_t alignedAddr;
#endif

    if (SD_CheckStatusWithTimeout(SD_TIMEOUT) < 0) {
        return res;
    }

#if defined(ENABLE_SCRATCH_BUFFER)
    if (!((uint32_t)buff & 0x3)) {
#endif
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)

        /*
        the SCB_CleanDCache_by_Addr() requires a 32-Byte aligned address
        adjust the address and the D-Cache size to clean accordingly.
        */
        alignedAddr = (uint32_t)buff & ~0x1F;
        SCB_CleanDCache_by_Addr((uint32_t *)alignedAddr, count * BLOCKSIZE + ((uint32_t)buff - alignedAddr));
#endif

        // LOG("SD:W:DMA(%u,%u,%d)\n",buff,sector,count);
        // GPIOD->BSRR |= GPIO_PIN_7;
        uint32_t intmask = __get_BASEPRI();
        __set_BASEPRI(1);
        if (BSP_SD_WriteBlocks_DMA((uint32_t *)buff, (uint32_t)(sector), count) == MSD_OK) {
            __set_BASEPRI(intmask);
            /* Wait that writing process is completed or a timeout occurs */
            // LOG("SD_write_dma: Waiting for DMA callback\n",0);
            timeout = HAL_GetTick();
            while ((WriteStatus == 0) && ((HAL_GetTick() - timeout) < SD_TIMEOUT)) {
            }
            /* in case of a timeout return error */
            if (WriteStatus == 0) {
                res = RES_ERROR;
                // LOG("[!] SD_write_dma: DMA callback timeout\n",0);
            } else {
                WriteStatus = 0;
                timeout = HAL_GetTick();
                // LOG("SD_write_dma: Ok. Getting card state\n",0);
                while ((HAL_GetTick() - timeout) < SD_TIMEOUT) {
                    if (BSP_SD_GetCardState() == SD_TRANSFER_OK) {
                        res = RES_OK;
                        break;
                    }
                }
            }
        } else {
            // LOG("[!] SD_write_dma: BSP_SD_WriteBlocks_DMA returned ERROR\n", 0);
        }
        // GPIOD->BSRR |= GPIO_PIN_7 << 16;
#if defined(ENABLE_SCRATCH_BUFFER)
    } else {
        /* Slow path, fetch each sector a part and memcpy to destination buffer */
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
        /*
         * invalidate the scratch buffer before the next write to get the actual data instead of the cached one
         */
        SCB_InvalidateDCache_by_Addr((uint32_t *)scratch, BLOCKSIZE);
#endif

        for (i = 0; i < count; i++) {
            WriteStatus = 0;

            memcpy((void *)scratch, (void *)buff, BLOCKSIZE);
            buff += BLOCKSIZE;

            ret = BSP_SD_WriteBlocks_DMA((uint32_t *)scratch, (uint32_t)sector++, 1);
            if (ret == MSD_OK) {
                /* wait for a message from the queue or a timeout */
                timeout = HAL_GetTick();
                while ((WriteStatus == 0) && ((HAL_GetTick() - timeout) < SD_TIMEOUT)) {
                }
                if (WriteStatus == 0) {
                    break;
                }

            } else {
                break;
            }
        }
        if ((i == count) && (ret == MSD_OK))
            res = RES_OK;
    }
#endif
    __set_BASEPRI(intmask);
    return res;
}

#endif /* _USE_WRITE == 1 */

/**
 * @brief  I/O control operation
 * @param  lun : not used
 * @param  cmd: Control code
 * @param  *buff: Buffer to send/receive control data
 * @retval DRESULT: Operation result
 */
#if _USE_IOCTL == 1

DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff) {
    DRESULT res = RES_ERROR;
    BSP_SD_CardInfo CardInfo;

    if (Stat & STA_NOINIT)
        return RES_NOTRDY;

    switch (cmd) {
        /* Make sure that no pending write process */
        case CTRL_SYNC:
            res = RES_OK;
            break;

            /* Get number of sectors on the disk (DWORD) */
        case GET_SECTOR_COUNT:
            BSP_SD_GetCardInfo(&CardInfo);
            *(DWORD *)buff = CardInfo.LogBlockNbr;
            res = RES_OK;
            break;

            /* Get R/W sector size (WORD) */
        case GET_SECTOR_SIZE:
            BSP_SD_GetCardInfo(&CardInfo);
            *(WORD *)buff = CardInfo.LogBlockSize;
            res = RES_OK;
            break;

            /* Get erase block size in unit of sector (DWORD) */
        case GET_BLOCK_SIZE:
            BSP_SD_GetCardInfo(&CardInfo);
            *(DWORD *)buff = CardInfo.LogBlockSize / SD_DEFAULT_BLOCK_SIZE;
            res = RES_OK;
            break;

        default:
            res = RES_PARERR;
    }

    return res;
}

#endif /* _USE_IOCTL == 1 */

/**
 * @brief Tx Transfer completed callbacks
 * @param hsd: SD handle
 * @retval None
 */
void BSP_SD_WriteCpltCallback(void) {

    WriteStatus = 1;
}

/**
 * @brief Rx Transfer completed callbacks
 * @param hsd: SD handle
 * @retval None
 */
void BSP_SD_ReadCpltCallback(void) {
    ReadStatus = 1;
}

/*==============================================================================================
  depending on the SD_HAL_Driver version, either the HAL_SD_ErrorCallback() or HAL_SD_AbortCallback()
  or both could be defined, activate the callbacks below when suitable and needed
==============================================================================================*/
void BSP_SD_AbortCallback(void) {
    // LOG("BSP_SD_AbortCallback\n", NULL);
}

void BSP_SD_ErrorCallback(void) {
    // LOG("BSP_SD_ErrorCallback\n", NULL);
}

/* USER CODE BEGIN lastSection */
/* can be used to modify / undefine previous code or add new code */
/* USER CODE END lastSection */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
