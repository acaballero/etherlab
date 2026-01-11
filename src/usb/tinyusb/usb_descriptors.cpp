/**
 * usb_descriptors_updated.c
 *
 * USB descriptors for composite device with MONO audio
 * CDC + MSC + Audio (mono microphone + mono speaker for WSJT-X)
 */

#include "../../lib/tinyusb/src/tusb.h"
#include "device/usbd.h"
#include "class/audio/audio.h"
#include "stm32f4xx.h"
#include <string.h>

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+

tusb_desc_device_t const desc_device = {.bLength = sizeof(tusb_desc_device_t),
                                        .bDescriptorType = TUSB_DESC_DEVICE,
                                        .bcdUSB = 0x0200,
                                        .bDeviceClass = TUSB_CLASS_MISC,
                                        .bDeviceSubClass = MISC_SUBCLASS_COMMON,
                                        .bDeviceProtocol = MISC_PROTOCOL_IAD,
                                        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

                                        .idVendor = 0x0483,  // STMicroelectronics
                                        .idProduct = 0x5741, // Unique PID for this device
                                        .bcdDevice = 0x0100,

                                        .iManufacturer = 0x01,
                                        .iProduct = 0x02,
                                        .iSerialNumber = 0x03,

                                        .bNumConfigurations = 0x01};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor - CDC + MSC + UAC2.0 Microphone (MONO)
//--------------------------------------------------------------------+
enum { ITF_NUM_CDC = 0, ITF_NUM_CDC_DATA, ITF_NUM_MSC, ITF_NUM_AUDIO_CONTROL, ITF_NUM_AUDIO_STREAMING, ITF_NUM_TOTAL };

#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT 0x02
#define EPNUM_CDC_IN 0x82
#define EPNUM_MSC_OUT 0x03
#define EPNUM_MSC_IN 0x83
#define EPNUM_AUDIO_IN 0x84

// TUD_AUDIO20_MIC_ONE_CH_DESC_LEN exists in TinyUSB 0.20.0!
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MSC_DESC_LEN + TUD_AUDIO20_MIC_ONE_CH_DESC_LEN)

uint8_t const desc_fs_configuration[] = {TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 500),

                                         TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

                                         TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),

                                         // UAC2.0 Mono Microphone - THIS MACRO EXISTS IN 0.20.0!
                                         TUD_AUDIO20_MIC_ONE_CH_DESCRIPTOR(
                                             /*_itfnum*/ ITF_NUM_AUDIO_CONTROL,
                                             /*_stridx*/ 6,
                                             /*_nBytesPerSample*/ CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX,
                                             /*_nBitsUsedPerSample*/ CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX * 8,
                                             /*_epin*/ EPNUM_AUDIO_IN,
                                             /*_epsize*/ CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX)};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_fs_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+
char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04}, "STMicroelectronics", "SDR Transceiver", "123456", "CAT Control", "SD Card", "Audio",
};

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
        uid[2] = HAL_GetUIDw2();

        chr_count = 0;
        for (int i = 0; i < 12 && chr_count < 31; i++) {
            uint8_t nibble = (uid[i / 8] >> ((i % 8) * 4)) & 0x0F;
            _desc_str[1 + chr_count++] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        }
    } else {
        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
            return NULL;
        const char *str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31)
            chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return _desc_str;
}
