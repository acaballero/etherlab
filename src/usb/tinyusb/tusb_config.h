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
#define USB_AUDIO_SAMPLE_RATE 48000
// Tell TinyUSB which USB peripheral to use
// STM32F427 has USB_OTG_HS which we use in Full Speed mode
#define CFG_TUSB_RHPORT1_BASE USB_OTG_HS_PERIPH_BASE

#define BOARD_TUD_RHPORT 1
#define BOARD_TUD_MAX_SPEED OPT_MODE_FULL_SPEED

//--------------------------------------------------------------------
// Common Configuration
//--------------------------------------------------------------------

#define CFG_TUSB_MCU OPT_MCU_STM32F4

#define CFG_TUSB_OS OPT_OS_NONE

#define CFG_TUSB_DEBUG 1

#define CFG_TUSB_DEBUG_PRINTF printf_

// Enable device stack
#define CFG_TUD_ENABLED 1
#define CFG_TUSB_RHPORT1_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

//--------------------------------------------------------------------
// Device Configuration
//------------------------------------------------- -------------------

#define CFG_TUD_ENDPOINT0_SIZE 64

//------------- Class enabled -------------//
#define CFG_TUD_CDC 1   // CAT protocol
#define CFG_TUD_MSC 1   // SD card
#define CFG_TUD_AUDIO 1 // USB audio input/output (1 function, 2 endponints (mic and speaker))

//------------- Audio Configuration -------------//
// Mono sound configuration

// Microphone (RX - Radio to PC)

#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT 1
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ 64

#define CFG_TUD_AUDIO_ENABLE_EP_IN 1
#define CFG_TUD_AUDIO_FUNC_1_ENABLE_EP_IN 1
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX 1
#define CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX 2
#define CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE USB_AUDIO_SAMPLE_RATE

// Speaker (TX - PC to Radio) -

#define CFG_TUD_AUDIO_ENABLE_EP_OUT 0
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX 1 // Mono
#define CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX 2

// Buffer sizes
#define CFG_TUD_AUDIO_EP_SZ_IN                                                                                                                                 \
    TUD_AUDIO_EP_SIZE(TUD_OPT_HIGH_SPEED, CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE, CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX, CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX)
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX CFG_TUD_AUDIO_EP_SZ_IN
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ (TUD_OPT_HIGH_SPEED ? 128 : 16) * CFG_TUD_AUDIO_EP_SZ_IN

#define CFG_TUD_AUDIO_EP_SZ_OUT                                                                                                                                \
    TUD_AUDIO_EP_SIZE(TUD_OPT_HIGH_SPEED, CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE, CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX, CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX)
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX CFG_TUD_AUDIO_EP_SZ_OUT
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ (TUD_OPT_HIGH_SPEED ? 128 : 16) * CFG_TUD_AUDIO_EP_SZ_OUT

//------------- CDC Configuration -------------//
#define CFG_TUD_CDC_RX_BUFSIZE 512
#define CFG_TUD_CDC_TX_BUFSIZE 512

//------------- MSC Configuration -------------//
#define CFG_TUD_MSC_EP_BUFSIZE 512

#ifdef __cplusplus
}
#endif

#endif /* TUSB_CONFIG_H */
