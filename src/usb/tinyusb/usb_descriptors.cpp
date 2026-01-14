/**
 * usb_descriptors_updated.c
 *
 * USB descriptors for composite device with MONO audio
 * CDC + MSC + Audio (mono microphone + mono speaker for WSJT-X)
 */

#include "../../lib/tinyusb/src/tusb.h"
#include "device/usbd.h"
#include "class/audio/audio.h"
#include "fatfs/fatfs.h"
#include "hw/stm32f4xx/connectivity.h"
#include "status.h"
#include "stm32f4xx.h"
#include "tinyusb/usb_composite_device.h"
#include <string.h>

// Track MSC state
static bool msc_enabled = false;

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]     AUDIO | MIDI | HID | MSC | CDC          [LSB]
 */
#define PID_MAP(itf, n) ((CFG_TUD_##itf) ? (1 << (n)) : 0)
#define USB_PID (0x4000 | PID_MAP(CDC, 0) | PID_MAP(MSC, 1) | PID_MAP(HID, 2) | PID_MAP(MIDI, 3) | PID_MAP(AUDIO, 4) | PID_MAP(VENDOR, 5))

tusb_desc_device_t const desc_device = {.bLength = sizeof(tusb_desc_device_t),
                                        .bDescriptorType = TUSB_DESC_DEVICE,
                                        .bcdUSB = 0x0200,
                                        .bDeviceClass = TUSB_CLASS_MISC,
                                        .bDeviceSubClass = MISC_SUBCLASS_COMMON,
                                        .bDeviceProtocol = MISC_PROTOCOL_IAD,
                                        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

                                        .idVendor = 0x0483,   // STMicroelectronics
                                        .idProduct = USB_PID, // Unique PID for this device
                                        .bcdDevice = 0x0100,

                                        .iManufacturer = 0x01,
                                        .iProduct = 0x02,
                                        .iSerialNumber = 0x03,

                                        .bNumConfigurations = 0x01};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

// Interface numbers when MSC is ENABLED
enum { ITF_NUM_CDC = 0, ITF_NUM_CDC_DATA, ITF_NUM_MSC, ITF_NUM_AUDIO_CONTROL, ITF_NUM_AUDIO_STREAMING, ITF_NUM_TOTAL_WITH_MSC };

// Interface numbers when MSC is DISABLED
enum { ITF_NUM_CDC_NO_MSC = 0, ITF_NUM_CDC_DATA_NO_MSC, ITF_NUM_AUDIO_CONTROL_NO_MSC, ITF_NUM_AUDIO_STREAMING_NO_MSC, ITF_NUM_TOTAL_NO_MSC };

// Note input endpoints (Board to PC) are assigned codes from 0x80 (msb bit set)
#define EPNUM_CDC_NOTIF 0x83
#define EPNUM_CDC_OUT 0x02
#define EPNUM_CDC_IN 0x82
#define EPNUM_AUDIO_IN 0x81
#define EPNUM_AUDIO_OUT 0x03
#define EPNUM_MSC_OUT 0x04
#define EPNUM_MSC_IN 0x84
//--------------------------------------------------------------------+
// Configuration Descriptor - CDC + MSC + UAC2.0 Microphone (MONO)
//--------------------------------------------------------------------+

#define CONFIG_WITH_MSC_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MSC_DESC_LEN + CFG_TUD_AUDIO * TUD_AUDIO20_MIC_ONE_CH_DESC_LEN)

uint8_t const desc_config_with_msc[] = {
    // Config descriptor
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL_WITH_MSC, 0, CONFIG_WITH_MSC_LEN, 0x00, 500),

    // CDC
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

    // MSC - Mass Storage
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),

    // Audio microphone
    TUD_AUDIO20_MIC_ONE_CH_DESCRIPTOR(ITF_NUM_AUDIO_CONTROL, 6, CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX, CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX * 8,
                                      EPNUM_AUDIO_IN, CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX),
};

//--------------------------------------------------------------------+
// Configuration WITHOUT MSC (CDC + Audio only)
//--------------------------------------------------------------------+

#define CONFIG_NO_MSC_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + CFG_TUD_AUDIO * TUD_AUDIO20_MIC_ONE_CH_DESC_LEN)

uint8_t const desc_config_no_msc[] = {
    // Config descriptor
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL_NO_MSC, 0, CONFIG_NO_MSC_LEN, 0x00, 500),

    // CDC
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_NO_MSC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

    // Audio microphone
    TUD_AUDIO20_MIC_ONE_CH_DESCRIPTOR(ITF_NUM_AUDIO_CONTROL_NO_MSC, 0, CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX,
                                      CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX * 8, EPNUM_AUDIO_IN, CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX),
};

//--------------------------------------------------------------------+
// Return correct descriptor based on MSC state
//--------------------------------------------------------------------+

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;

    if (msc_enabled) {
        return desc_config_with_msc;
    } else {
        return desc_config_no_msc;
    }
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+
char const *string_desc_arr[] = {(const char[]){0x09, 0x04}, "Etherlab", "EL24 SDR Transceiver", "123456", "CAT Control", "MSC storage", "Audio"};

static uint16_t _desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else if (index == 3) {
        uint32_t uid[3];
        uid[0] = HAL_GetUIDw0();
        uid[1] = HAL_GetUIDw1();
        uid[2] = HAL_GetUIDw2() + 2;

        chr_count = 0;
        for (int i = 0; i < 12 && chr_count < 31; i++) {
            uint8_t nibble = (uid[i / 8] >> ((i % 8) * 4)) & 0x0F;
            _desc_str[1 + chr_count++] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        }
    } else {
        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) {
            return NULL;
        }
        const char *str = string_desc_arr[index];
        chr_count = strlen(str);
        size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1;
        if (chr_count > max_count) {
            chr_count = max_count;
        }
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return _desc_str;
}

//--------------------------------------------------------------------+
// Public API to enable/disable MSC
//--------------------------------------------------------------------+

// Call this to trigger re-enumeration with new descriptor
void usb_trigger_reenumeration(void) {
    // Disconnect from bus
    tud_disconnect();

    // Wait a bit
    HAL_Delay(100);

    // Reconnect - will use new descriptor
    tud_connect();
}

// Must be called when USB is NOT connected
bool usb_set_msc_enabled(bool enable) {

    if (msc_enabled == enable) {
        return false;
    }

    if (lock_sd_card(5000)) { // Wait for SD card to be free
        restart_sdio(
            true); // FIXME: Using SDIO at high speed here does not increase the SD speed. The MSC usb interface does not use DMA which is a bottleneck.
                   // However, I've tried enabling DMA for MSC operation and it seems to mess with the USB DMA or something (dindn't try much)

        msc_enabled = enable;

        if (tud_mounted()) {
            // USB already connected: re-enumerate
            usb_trigger_reenumeration();
        }

        return true;
    } else {
        status::pop_alert(status::ERROR, "Timeout waiting for SD card");
    }

    return false;
}

bool usb_get_msc_enabled(void) {
    return msc_enabled;
}
