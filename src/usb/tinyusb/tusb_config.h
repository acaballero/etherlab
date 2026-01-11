/**
 * tusb_config.h
 *
 * TinyUSB configuration for STM32F427
 * Composite device: CDC + MSC + Audio
 */

#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// Board Specific Configuration
//--------------------------------------------------------------------+

#define BOARD_TUD_RHPORT 0
#define BOARD_TUD_MAX_SPEED OPT_MODE_FULL_SPEED

//--------------------------------------------------------------------
// Common Configuration
//--------------------------------------------------------------------

#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU OPT_MCU_STM32F4
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

// Enable device stack
#define CFG_TUD_ENABLED 1
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

//--------------------------------------------------------------------
// Device Configuration
//------------------------------------------------- -------------------

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE 64
#endif

//------------- Class enabled -------------//
#define CFG_TUD_CDC 1   // CAT protocol
#define CFG_TUD_MSC 1   // SD card
#define CFG_TUD_AUDIO 1 // USB audio input/output

//------------- Audio Configuration -------------//
// Using stereo (TinyUSB limitation), but we'll send mono data to both channels

#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN TUD_AUDIO_HEADSET_STEREO_DESC_LEN
#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT 2
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ 64

// Microphone (RX - Radio to PC) - using stereo descriptor
#define CFG_TUD_AUDIO_ENABLE_EP_IN 1
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX 2 // Stereo (TinyUSB requirement)
#define CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX 2
#define CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE 48000

// Speaker (TX - PC to Radio) - using stereo descriptor
#define CFG_TUD_AUDIO_ENABLE_EP_OUT 1
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX 1 // Mono sound
#define CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX 2

// Buffer sizes
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ                                                                                                                   \
    (CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE / 1000 * CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX) * 4

#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX                                                                                                                      \
    (CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE / 1000 * CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX) + 1

#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ                                                                                                                  \
    (CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE / 1000 * CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX) * 4

#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX                                                                                                                     \
    (CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE / 1000 * CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX) + 1

#define CFG_TUD_AUDIO_FUNC_1_N_FORMATS 1
#define CFG_TUD_AUDIO_FUNC_1_CHANNEL_MAP_RX AUDIO_CHANNEL_CONFIG_NON_PREDEFINED
#define CFG_TUD_AUDIO_FUNC_1_CHANNEL_MAP_TX AUDIO_CHANNEL_CONFIG_NON_PREDEFINED

//------------- CDC Configuration -------------//
#define CFG_TUD_CDC_RX_BUFSIZE 512
#define CFG_TUD_CDC_TX_BUFSIZE 512

//------------- MSC Configuration -------------//
#define CFG_TUD_MSC_EP_BUFSIZE 512

#ifdef __cplusplus
}
#endif

#endif /* TUSB_CONFIG_H */
