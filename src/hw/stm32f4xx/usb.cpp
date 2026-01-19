//
// Created by Angel Dust on 15/06/2024.
//

#include "usb.h"
#include "../../lib/tinyusb/src/tusb.h"
#include "fatfs/fatfs.h"
#include "hw/stm32f4xx/connectivity.h"
#include "status.h"
#include "tinyusb/tusb_config.h"
#include "tinyusb/usb_composite_device.h"
#include <type_traits>

extern PCD_HandleTypeDef hpcd_USB_OTG_HS;

volatile uint8_t usb_msc_active = 0;

/**
 * @brief This function handles USB On The Go HS global interrupt.
 */
extern "C" void OTG_HS_IRQHandler(void) {
    /* USER CODE BEGIN OTG_HS_IRQn 0 */

    /* USER CODE END OTG_HS_IRQn 0 */
    // HAL_PCD_IRQHandler(&hpcd_USB_OTG_HS);
    dcd_int_handler(BOARD_TUD_RHPORT);
    /* USER CODE BEGIN OTG_HS_IRQn 1 */

    /* USER CODE END OTG_HS_IRQn 1 */
}

void MX_USB_OTG_HS_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE(); // Enable USB clock
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /**USB_OTG_HS GPIO Configuration
       PB13     ------> USB_OTG_HS_VBUS
       PB14     ------> USB_OTG_HS_DM
       PB15     ------> USB_OTG_HS_DP
       */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_OTG_HS_FS;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Disable VBUS sensing (required when using internal PHY without VBUS pin)
    USB_OTG_HS->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;
    USB_OTG_HS->GCCFG &= ~USB_OTG_GCCFG_VBUSBSEN;
    USB_OTG_HS->GCCFG &= ~USB_OTG_GCCFG_VBUSASEN;

    // Power up the internal PHY
    USB_OTG_HS->GCCFG |= USB_OTG_GCCFG_PWRDWN;

    // Wait for PHY clock to stabilize
    HAL_Delay(20);

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(OTG_HS_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
}

uint8_t getUSBConnectionStatus() {
    return usb_connected() ? USB_CONN_STATUS_CONNECTED : USB_CONN_STATUS_DISCONNECTED;
}

// bool init_USB_MSC() {

//     if (usb_msc_active) {
//         return true;
//     }

//     if (lock_sd_card(5000)) { // Wait for SD card to be free

//         restart_sdio(
//             true); // FIXME: Using SDIO at high speed here does not increase the SD speed. The MSC usb interface does not use DMA which is a bottleneck.
//                    // However, I've tried enabling DMA for MSC operation and it seems to mess with the USB DMA or something (dindn't try much)

//         USB_SetupMSC();

//         usb_msc_active = 1;

//         return true;
//     } else {
//         status::pop_alert(status::ERROR, "Timeout waiting for SD card");
//         return false;
//     }
// }

// bool init_USB_CDC() {

//     restart_sdio(true);

//     USB_SetupCDC();

//     usb_msc_active = 0;

//     return true;
// }
