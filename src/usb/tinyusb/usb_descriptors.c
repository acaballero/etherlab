/**
 * usb_descriptors_updated.c
 *
 * USB descriptors for composite device with MONO audio
 * CDC + MSC + Audio (mono microphone + mono speaker for WSJT-X)
 */

#include "../../lib/tinyusb/src/tusb.h"

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
// Configuration Descriptor
//--------------------------------------------------------------------+

enum { ITF_NUM_CDC_0 = 0, ITF_NUM_CDC_0_DATA, ITF_NUM_MSC, ITF_NUM_AUDIO_CONTROL, ITF_NUM_AUDIO_STREAMING_MIC, ITF_NUM_AUDIO_STREAMING_SPK, ITF_NUM_TOTAL };

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MSC_DESC_LEN + TUD_AUDIO_MIC_ONE_CH_DESC_LEN)

#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT 0x02
#define EPNUM_CDC_IN 0x82

#define EPNUM_MSC_OUT 0x03
#define EPNUM_MSC_IN 0x83

#define EPNUM_AUDIO_OUT 0x04
#define EPNUM_AUDIO_IN 0x84

uint8_t const desc_fs_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 500),

    // Interface number, string index, EP notification address and size, EP data address (out, in) and size
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),

    // Audio descriptor - MONO microphone (RX: Radio->PC) and MONO speaker (TX: PC->Radio)
    TUD_AUDIO_MIC_ONE_CH_DESCRIPTOR(
        /*_itfnum*/ ITF_NUM_AUDIO_CONTROL,
        /*_stridx*/ 6,
        /*_nBytesPerSample*/ CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX,
        /*_nBitsUsedPerSample*/ CFG_TUD_AUDIO_FUNC_1_RESOLUTION_RX,
        /*_epout*/ EPNUM_AUDIO_OUT,
        /*_epoutsize*/ CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX,
        /*_epin*/ EPNUM_AUDIO_IN,
        /*_epinsize*/ CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX)};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_fs_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: English (0x0409)
    "STMicroelectronics",       // 1: Manufacturer
    "SDR Transceiver",          // 2: Product
    "123456",                   // 3: Serial (will be overwritten)
    "CAT Control",              // 4: CDC Interface
    "SD Card",                  // 5: MSC Interface
    "Audio",                    // 6: Audio Interface
};

static uint16_t _desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;

    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else if (index == 3) {
        // Get unique serial number from chip UID
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

    // Header
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}
