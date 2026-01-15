/**
 * usb_composite_device.h
 *
 * TinyUSB composite device interface
 * Replaces HAL USB functions
 */

#ifndef USB_COMPOSITE_DEVICE_H
#define USB_COMPOSITE_DEVICE_H

#include "os/periodic_task.h"

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

//--------------------------------------------------------------------+
// MSC Enable/Disable API
//--------------------------------------------------------------------+

/**
 * @brief Enable or disable MSC (Mass Storage Class)
 *
 * This allows user to choose whether SD card is exposed via USB.
 *
 * IMPORTANT: Can only be called when USB is disconnected.
 * If USB is already connected, this will return false.
 *
 * To change while connected:
 * 1. Call usb_set_msc_enabled(true/false)
 * 2. Call usb_trigger_reenumeration()
 *
 * @param enable  true to enable MSC, false to disable
 * @return true if successful, false if USB is currently connected
 */
bool usb_set_msc_enabled(bool enable);

/**
 * @brief Get current MSC enable state
 * @return true if MSC is enabled, false if disabled
 */
bool usb_get_msc_enabled(void);

/**
 * @brief Trigger USB re-enumeration
 *
 * Disconnects and reconnects USB, forcing PC to re-enumerate
 * with the new configuration descriptor (MSC enabled or disabled).
 *
 * Use after calling usb_set_msc_enabled() while connected.
 */
void usb_trigger_reenumeration(void);

// SD Card interface functions you need to implement
// These wrap your existing SD card code
int sd_card_read_blocks(uint32_t lba, uint8_t *buffer, uint32_t block_count);
int sd_card_write_blocks(uint32_t lba, const uint8_t *buffer, uint32_t block_count);
uint32_t sd_card_get_block_count(void);
bool sd_card_is_ready(void);

#ifdef __cplusplus
}
#endif

namespace usb {
extern os::periodic_task task;
}
#endif // USB_COMPOSITE_DEVICE_H
