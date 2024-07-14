//
// Created by Angel Dust on 15/06/2024.
//

#include "usb.h"
#include "usb/usbd_conf.h"
#include "usbd_def.h"

extern PCD_HandleTypeDef hpcd_USB_OTG_HS;


/**
  * @brief This function handles USB On The Go HS global interrupt.
  */
void OTG_HS_IRQHandler(void)
{
    /* USER CODE BEGIN OTG_HS_IRQn 0 */

    /* USER CODE END OTG_HS_IRQn 0 */
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_HS);
    /* USER CODE BEGIN OTG_HS_IRQn 1 */

    /* USER CODE END OTG_HS_IRQn 1 */
}


uint8_t getConnectionStatus() {
    return  ((((USBD_HandleTypeDef *)hpcd_USB_OTG_HS.pData)->dev_state) == USBD_STATE_CONFIGURED) ? USB_CONN_STATUS_CONNECTED : USB_CONN_STATUS_DISCONNECTED;
}