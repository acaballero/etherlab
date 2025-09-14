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

bool init_USB_MSC() {

    if (usb_msc_active) {
        return true;
    }

    if (lock_sd_card(5000)) { // Wait for SD card to be free

        USB_SetupMSC();

        restart_sdio(true);

        usb_msc_active = 1;

        return true;
    }

    status::pop_alert(status::ST_ERROR, "Timeout waiting for SD card");

    return false;
}

bool init_USB_CDC() {

    restart_sdio(true);

    USB_SetupCDC();

    usb_msc_active = 0;

    return true;
}
