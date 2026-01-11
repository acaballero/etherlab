/**
 * usb_composite_device.h
 *
 * TinyUSB composite device interface
 * Replaces HAL USB functions
 */

#ifndef USB_COMPOSITE_DEVICE_H
#define USB_COMPOSITE_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Initialize USB composite device (CDC+MSC+Audio)
// Call this instead of USB_SetupCDC() or USB_SetupMSC()
void usb_composite_init(void);

// USB task - call frequently from main loop
// Replaces any HAL USB polling
void usb_composite_task(void);

// CDC functions - for your CAT protocol
bool usb_cdc_transmit(const uint8_t *data, uint16_t len); // Replaces CDC_Transmit_HS()
bool usb_composite_cdc_connected(void);

// MSC status
bool usb_composite_msc_connected(void);

// SD Card interface functions you need to implement
// These wrap your existing SD card code
int sd_card_read_blocks(uint32_t lba, uint8_t *buffer, uint32_t block_count);
int sd_card_write_blocks(uint32_t lba, const uint8_t *buffer, uint32_t block_count);
uint32_t sd_card_get_block_count(void);
bool sd_card_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif // USB_COMPOSITE_DEVICE_H
