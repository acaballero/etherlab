

#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_msc.h"
#include "usbd_storage_if.h"
#include "ff.h"

USBD_HandleTypeDef hUsbDeviceHS;

extern void Error_Handler();

void USB_SetupCDC(void) {
    if (hUsbDeviceHS.dev_state) { // Already initalized? Stopping an unitialized device crashes
        USBD_Stop(&hUsbDeviceHS);
        USBD_DeInit(&hUsbDeviceHS);
    }

    /* Init Device Library, add supported class and start the library. */
    if (USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_RegisterClass(&hUsbDeviceHS, &USBD_CDC) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_CDC_RegisterInterface(&hUsbDeviceHS, &USBD_Interface_fops_HS) != USBD_OK) {
        Error_Handler();
    }

    if (USBD_Start(&hUsbDeviceHS) != USBD_OK) {
        Error_Handler();
    }
}

void USB_SetupMSC(void) {

    if (hUsbDeviceHS.dev_state) { // Already initalized? Stopping an unitialized device crashes
        USBD_Stop(&hUsbDeviceHS);
        USBD_DeInit(&hUsbDeviceHS);
    }

    // Unmount FATFS if case is previously mounted
    f_mount(NULL, "", 0);

    // Re-init as MSC

    if (USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_RegisterClass(&hUsbDeviceHS, &USBD_MSC) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_MSC_RegisterStorage(&hUsbDeviceHS, &USBD_Storage_Interface_fops_HS) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_Start(&hUsbDeviceHS) != USBD_OK) {
        Error_Handler();
    }
}
