/**
 * usb_composite_device.h
 *
 * TinyUSB composite device interface
 * Replaces HAL USB functions
 */

#ifndef USB_COMPOSITE_DEVICE_H
#define USB_COMPOSITE_DEVICE_H

#include "os/periodic_task.h"
#include "tinyusb/tusb_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Interface numbers when MSC is ENABLED
enum {
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_MSC,
    ITF_NUM_AUDIO_CONTROL,
    ITF_NUM_AUDIO_STREAMING,
    ITF_NUM_SPK_CONTROL,
    ITF_NUM_SPK_STREAMING,
    ITF_NUM_TOTAL_WITH_MSC
};

// Interface numbers when MSC is DISABLED
enum {
    ITF_NUM_CDC_NO_MSC = 0,
    ITF_NUM_CDC_DATA_NO_MSC,
    ITF_NUM_AUDIO_CONTROL_NO_MSC,
    ITF_NUM_AUDIO_STREAMING_NO_MSC,
    ITF_NUM_SPK_CONTROL_NO_MSC,
    ITF_NUM_SPK_STREAMING_NO_MSC,
    ITF_NUM_TOTAL_NO_MSC
};

enum { ITF_IX_MICROPHONE, ITF_IX_SPEAKER };

// Initialize USB composite device (CDC+MSC+Audio)
// Call this instead of USB_SetupCDC() or USB_SetupMSC()
void usb_composite_init(void);

bool usb_connected();

bool usb_cable_connected();

// USB task - call frequently from main loop
// Replaces any HAL USB polling
void usb_composite_task(void);

// CDC functions - for your CAT protocol
bool usb_cdc_transmit(const uint8_t *data, uint16_t len); // Replaces CDC_Transmit_HS()
bool usb_cdc_connected(void);

// MSC status
bool usb_msc_connected(void);

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

// There are mute and volume arrays (per channel) for each audio interface

extern bool mute[CFG_TUD_AUDIO][CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; // +1 for master channel 0
extern int16_t volume_db[CFG_TUD_AUDIO][CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];
extern float volume_factor[CFG_TUD_AUDIO][CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];

} // namespace usb
#endif // USB_COMPOSITE_DEVICE_H
