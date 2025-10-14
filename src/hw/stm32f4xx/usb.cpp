//
// Created by Angel Dust on 15/06/2024.
//

#include "usb.h"
#include "fatfs/fatfs.h"
#include "hw/stm32f4xx/connectivity.h"
#include "status.h"
#include "usb/usbd_conf.h"
#include "usbd_def.h"
#include "usb/usb_device.h"
#include "usbd_msc.h"
#include <type_traits>

extern PCD_HandleTypeDef hpcd_USB_OTG_HS;

volatile uint8_t usb_msc_active = 0;

/**
 * @brief This function handles USB On The Go HS global interrupt.
 */
void OTG_HS_IRQHandler(void) {
    /* USER CODE BEGIN OTG_HS_IRQn 0 */

    /* USER CODE END OTG_HS_IRQn 0 */
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_HS);
    /* USER CODE BEGIN OTG_HS_IRQn 1 */

    /* USER CODE END OTG_HS_IRQn 1 */
}

uint8_t getUSBConnectionStatus() {
    return ((((USBD_HandleTypeDef *)hpcd_USB_OTG_HS.pData)->dev_state) == USBD_STATE_CONFIGURED) ? USB_CONN_STATUS_CONNECTED : USB_CONN_STATUS_DISCONNECTED;
}

// Add this function to usb_device.c or connectivity.cpp
bool SD_Reinit_For_MSC(void) {
    // Stop any ongoing SD operations
    HAL_SD_Abort(&hsd);

    // Small delay to ensure clean state
    HAL_Delay(10);

    // Deinitialize SD card completely
    HAL_SD_DeInit(&hsd);

    // Reinitialize SD card hardware
    if (HAL_SD_Init(&hsd) != HAL_OK) {
        return false;
    }

    // Configure 4-bit bus width for better performance
    if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) != HAL_OK) {
        return false;
    }

    // Ensure DMA is properly configured by reinitializing the MSP
    HAL_SD_MspDeInit(&hsd);
    HAL_SD_MspInit(&hsd);

    // Wait for card to be ready
    uint32_t timeout = HAL_GetTick() + 1000;
    while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER && HAL_GetTick() < timeout) {
        HAL_Delay(10);
    }

    if (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {
        return false;
    }

    return true;
}

bool init_USB_MSC() {

    if (usb_msc_active) {
        return true;
    }

    if (lock_sd_card(5000)) { // Wait for SD card to be free

        restart_sdio(
            true); // FIXME: Using SDIO at high speed here does not increase the SD speed. The MSC usb interface does not use DMA which is a bottleneck.
                   // However, I've tried enabling DMA for MSC operation and it seems to mess with the USB DMA or something (dindn't try much)

        USB_SetupMSC();

        usb_msc_active = 1;

        return true;
    } else {
        status::pop_alert(status::ERROR, "Timeout waiting for SD card");
        return false;
    }
}

bool init_USB_CDC() {

    restart_sdio(true);

    USB_SetupCDC();

    usb_msc_active = 0;

    return true;
}
