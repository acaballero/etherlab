/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : usbd_storage_if.c
 * @version        : v1.0_Cube
 * @brief          : Memory management layer.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_storage_if.h"
#include "stm32f4xx_hal_def.h"
#include "usbd_def.h"
#include "fatfs/sd_diskio.h"

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
 * @brief Usb device.
 * @{
 */

/** @defgroup USBD_STORAGE
 * @brief Usb mass storage device module
 * @{
 */

/** @defgroup USBD_STORAGE_Private_TypesDefinitions
 * @brief Private types.
 * @{
 */

/* USER CODE BEGIN PRIVATE_TYPES */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_Defines
 * @brief Private defines.
 * @{
 */

/* USER CODE BEGIN PRIVATE_DEFINES */

/* USER CODE END PRIVATE_DEFINES */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_Macros
 * @brief Private macros.
 * @{
 */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_Variables
 * @brief Private variables.
 * @{
 */

/* USER CODE BEGIN INQUIRY_DATA_HS */
/** USB Mass storage Standard Inquiry Data. */
const int8_t STORAGE_Inquirydata_HS[] = {
    /* 36 */

    /* LUN 0 */
    0x00, 0x80, 0x02, 0x02, (STANDARD_INQUIRY_DATA_LEN - 5),
    0x00, 0x00, 0x00, 'S',  'T',
    'M',  ' ',  ' ',  ' ',  ' ',
    ' ', /* Manufacturer : 8 bytes */
    'P',  'r',  'o',  'd',  'u',
    'c',  't',  ' ', /* Product      : 16 Bytes */
    ' ',  ' ',  ' ',  ' ',  ' ',
    ' ',  ' ',  ' ',  '0',  '.',
    '0',  '1' /* Version      : 4 Bytes */
};
/* USER CODE END INQUIRY_DATA_HS */

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Exported_Variables
 * @brief Public variables.
 * @{
 */

extern USBD_HandleTypeDef hUsbDeviceHS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_FunctionPrototypes
 * @brief Private functions declaration.
 * @{
 */

static int8_t STORAGE_Init_HS(uint8_t lun);
static int8_t STORAGE_GetCapacity_HS(uint8_t lun, uint32_t *block_num, uint16_t *block_size);
static int8_t STORAGE_IsReady_HS(uint8_t lun);
static int8_t STORAGE_IsWriteProtected_HS(uint8_t lun);
static int8_t STORAGE_Read_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun_HS(void);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */

/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
 * @}
 */

USBD_StorageTypeDef USBD_Storage_Interface_fops_HS = {STORAGE_Init_HS, STORAGE_GetCapacity_HS, STORAGE_IsReady_HS,   STORAGE_IsWriteProtected_HS,
                                                      STORAGE_Read_HS, STORAGE_Write_HS,       STORAGE_GetMaxLun_HS, (int8_t *)STORAGE_Inquirydata_HS};

int8_t STORAGE_Init_HS(uint8_t lun) {

    return (USBD_OK);
}

int8_t STORAGE_GetCapacity_HS(uint8_t lun, uint32_t *block_num, uint16_t *block_size) {
    HAL_SD_CardInfoTypeDef cardInfo;
    HAL_SD_GetCardInfo(&hsd, &cardInfo);

    *block_num = cardInfo.LogBlockNbr;
    *block_size = cardInfo.LogBlockSize;
    return USBD_OK;
}

int8_t STORAGE_IsReady_HS(uint8_t lun) {
    return (HAL_SD_GetCardState(&hsd) == HAL_SD_CARD_TRANSFER) ? USBD_OK : USBD_FAIL;
}

int8_t STORAGE_IsWriteProtected_HS(uint8_t lun) {
    /* USER CODE BEGIN 12 */
    return (USBD_OK);
    /* USER CODE END 12 */
}

int8_t wait_for_sd_idle(uint32_t timeout_ms) {
    uint32_t start = HAL_GetTick();
    while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {
        if ((HAL_GetTick() - start) > timeout_ms) {
            return 0; // timeout
        }
    }
    return 1;
}

int8_t STORAGE_Read_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len) {
    volatile int8_t ret = (HAL_SD_ReadBlocks(&hsd, buf, blk_addr, blk_len, HAL_MAX_DELAY) == HAL_OK) ? USBD_OK : USBD_FAIL;

    return ret;
}

int8_t STORAGE_Write_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len) {
    volatile int8_t ret = HAL_SD_WriteBlocks(&hsd, buf, blk_addr, blk_len, HAL_MAX_DELAY);

    if (ret != HAL_OK) {
        return USBD_FAIL;
    } else if (wait_for_sd_idle(HAL_MAX_DELAY)) {
        return USBD_OK;
    } else {
        return USBD_BUSY;
    }
}

/*
int8_t STORAGE_Read_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len) {

    // Use DMA for better performance
    if (SD_read_dma(lun, buf, blk_addr, blk_len) != RES_OK) {
        return USBD_FAIL;
    }
}

int8_t STORAGE_Write_HS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len) {

    // Use DMA for better performance
    if (SD_write_dma(lun, buf, blk_addr, blk_len) != RES_OK) {
        return USBD_FAIL;
    }
}
*/

int8_t STORAGE_GetMaxLun_HS(void) {
    return 0; // single LUN
}
