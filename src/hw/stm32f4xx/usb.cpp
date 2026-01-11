//
// Created by Angel Dust on 15/06/2024.
//

#include "usb.h"
#include "../../lib/tinyusb/src/tusb.h"
#include "fatfs/fatfs.h"
#include "hw/stm32f4xx/connectivity.h"
#include "status.h"
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
    dcd_int_handler(0);
    /* USER CODE BEGIN OTG_HS_IRQn 1 */

    /* USER CODE END OTG_HS_IRQn 1 */
}

uint8_t getUSBConnectionStatus() {
    return tud_mounted() ? USB_CONN_STATUS_CONNECTED : USB_CONN_STATUS_DISCONNECTED;
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
