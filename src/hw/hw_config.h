//
// Created by Angel Dust on 01/11/2019.
//

#ifndef __HWCONFIG__
#define __HWCONFIG__

#define BOARD_VERSION 2

#if BOARD_VERSION == 1 // STM32F303 board

#include "board/board_v1.h"
#include "board/board_v1_config.h"
#include "board/board_v1_ui.h"

#else // STM32F497 board

#include "board/board_v2.h"
#include "board/board_v2_config.h"
#include "board/board_v2_ui.h"
#include "hw/stm32f4xx/rtc.h"

#endif

#include "dsp/fft/fft.h"

#if FFT_TYPE == FFT_TYPE_Q31

#define DMA_PALIGN DMA_PDATAALIGN_WORD
#define DMA_MALIGN DMA_MDATAALIGN_WORD

#else

#define DMA_PALIGN DMA_PDATAALIGN_HALFWORD
#define DMA_MALIGN DMA_MDATAALIGN_HALFWORD

#endif

#define ENABLE_FFT 1

#define ENABLE_RTC 1
#define DEBUG_SD_CARD 0
#define LCD_ENABLED 1

// Enable real-time DSP functions (capture, replay, demodulate...) Not suitable
// for low speed MCUs
#define DSP_ENABLED 1

// Enable USB serial interface (uses around 6kb of data space, 2Kb for Rx/Tx
// buffers plus some big structures)
#define USB_ENABLED 1

// Enable SWO pin for serial debugging
#define SWO_ENABLED 1
#define ENABLE_LOGGER 1

//#define FILTER_BANK_SHIFT_REG_SIZE 16 // There are 2 filter bank boards,
// chaining 2 8-bit shift registers.

#define CONFIG_AUTOSAVE_SECS 300 // Autosave config every CONFIG_AUTOSAVE_SECS seconds

#define V_REF 3.2f

/******************/
/* Rotary encoder */
/******************/

//#define ROTARY_ENCODER_mIN 0
//#define ROTARY_ENCODER_mAX 1000000L
#define ROTARY_ENCODER_mAX_STEP 100
#define ROTARY_ENCODER_mIN_STEP 1

/* #1 MCP23017 (right one in the expander PCB) Port expander bit positions */
// PORT_A
#define GPIOEXP_IF_FILTER_3KHZ 0
#define GPIOEXP_IF_FILTER_15KHZ 1
#define GPIOEXP_IF_FILTER_150KHZ 2
#define GPIOEXP_ANALOG_RXTX_DIGITAL_TX_SWITCH 3
/* Bits 4-7 are used in the frontend filter bank */

// PORT_B (inverted logic in pins 3 - 7)
#ifdef STM32F4xx
// The LNA, attenuator and pass-thru must be exclusively activated. Note the LNA
// has inverted logic
#define GPIOEXP_FRONT_THRU 1       // Frontend pass-thru
#define GPIOEXP_FRONT_ATTENUATOR 2 // Frontend attenuator
#define GPIOEXP_FM_MODULATOR 3     // Goes to PB-1 (+5v) of the power switch board (0 is on)
#define GPIOEXP_LNA 4              // Goes to PB-2 (+5v) of the power switch board (0 is on)
#define GPIOEXP_5VIF_RX 5          // 1st mixer board. Goes to PB-3 (+5v) of the power switch board (0 is on)
#define GPIOEXP_FM_DETECTOR 6      // Goes to PB-4 (+5v) of the power switch boad
#define GPIOEXP_AM_DETECTOR 7      // Goes to PB-5 (+5v) of the power switch boad
#else
#define GPIOEXP_70CM_AMP 0 // Goes to PB2CTRL (+9v) of the power switch boad (the one with 3906s for
// switching the power rail)
#define GPIOEXP_70CM_AMP_BYPASS 2
#define GPIOEXP_FM_MODULATOR 3 // Goes to PB-1 (+5v) of the power switch board (0 is on)
#define GPIOEXP_LNA 4          // Goes to PB-2 (+5v) of the power switch board (0 is on)
#define GPIOEXP_5VIF_RX 5      // 1st mixer board. Goes to PB-3 (+5v) of the power switch board (0 is on)
#define GPIOEXP_FM_DETECTOR 6  // Goes to PB-4 (+5v) of the power switch boad
#define GPIOEXP_AM_DETECTOR 7  // Goes to PB-5 (+5v) of the power switch boad
#endif

/* #2 MCP23017 Port expander bit positions (left one looking from power supply
 * input) */

// PORT_A
#define GPIOEXP_MUTE 0
#define GPIOEXP_AGC 1 // AGC on/off (output)
#define GPIOEXP_ALC 2 // ALC on/off (output)
#define GPIOEXP_RX 3  // +5v (20 ma.) RX (1) / TX (0)

#define GPIOEXP_ENABLE_POW_CTRL_SHIFT_REG 5
#define GPIOEXP_POW_AMP_BIAS 6
#define GPIOEXP_RSSI_LEVEL_ADAPTER 7 // Log amplifiers (FM & AM analog demodulators) signal strength voltage shifter

// PORT_B (normal logic)

//#define GPIOEXP_2ND_15KHZ_FILTER 2 // Second 15 Khz. IF filter switch
#define GPIOEXP_5VIF_TX 3      // +5v TX (1) / RX (0)
#define GPIOEXP_IF_RSSI_5V 4   // Bias for IF RSSI detector
#define GPIOEXP_5V_ANALOG_TX 5 // 5V bias for analog-mode TX boards
#define GPIOEXP_10MHHZ_MIXER 7

/* Power control bit weights */
#define POWCRL_PA1 (1 << 7)
#define POWCRL_PA2 (1 << 6)
#define POWCRL_PB1 (1 << 5)
#define POWCRL_PB2 (1 << 4)
#define POWCRL_PC1 (1 << 3)
#define POWCRL_PC2 (1 << 2)
#define POWCRL_P5 (1 << 1)
#define POWCRL_P12 1

/* #3 MCP23017 Front panel */

// PORT_A
//#define GPIOEXP_FPANEL_PAD_BUTTON_1 0
//#define GPIOEXP_FPANEL_PAD_BUTTON_2 1
//#define GPIOEXP_FPANEL_PAD_BUTTON_3 2
//#define GPIOEXP_FPANEL_PAD_BUTTON_4 3
//#define GPIOEXP_FPANEL_PAD_BUTTON_5 4
//#define GPIOEXP_FPANEL_PAD_BUTTON_6 5
#define GPIOEXP_FPANEL_TX_LED 6
//#define GPIOEXP_FPANEL_SPARE 7
// PORT_B
//#define GPIOEXP_FPANEL_DISPLAY_BUTTON_6 0
//#define GPIOEXP_FPANEL_DISPLAY_BUTTON_5 1
//#define GPIOEXP_FPANEL_DISPLAY_BUTTON_4 2
//#define GPIOEXP_FPANEL_DISPLAY_BUTTON_3 3
//#define GPIOEXP_FPANEL_DISPLAY_BUTTON_2 4
//#define GPIOEXP_FPANEL_DISPLAY_BUTTON_1 5
//#define GPIOEXP_FPANEL_BACKLIGHT 6
#define GPIOEXP_FPANEL_STBY_LED 7

#endif
