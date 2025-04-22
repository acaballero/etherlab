#pragma once

#ifndef TRX_FRONTEND_STM32_H
#define TRX_FRONTEND_STM32_H

// priority

#ifndef STM32F4xx
#define STM32F4xx
#endif

#ifdef STM32F4xx

// HSE VALUE defined by compiler parameters to avoid warnings
//#define HSE_VALUE ((uint32_t)26000000) /*!< Value of the External oscillator
// in Hz */
#define CPU_CORE_FREQUENCY_HZ SystemCoreClock
//#define CPU_TIMER_PRESCALER_MS_HZ 168000
#define CPU_TIMER_PRESCALER_MS_HZ 264000 // OVERCLOCKED (See clocks.cpp)

#define DISABLE_SD_INIT 1
#define ENABLE_SD_CARD 1

#include <stm32f4xx.h>
#include <stm32f4xx_hal_dac_ex.h>
#include "hw/stm32f4xx/gpio.h"
#include "hw/stm32f4xx/adc.h"
#include "hw/stm32f4xx/dac.h"
#include "hw/stm32f4xx/dma.h"
#include "hw/stm32f4xx/usb.h"
#include "hw/stm32f4xx/power.h"
#include "hw/stm32f4xx/timers.h"
#include "hw/stm32f4xx/flash.h"
#include "hw/stm32f4xx/clocks.h"
#include "hw/stm32f4xx/connectivity.h"
#include "fatfs/sd_diskio.h"
#include "fatfs/spi_diskio.h"

/* Board config linkage */
#define ST_HW_CONFIG st_hw_config

/* FATFS handle linkage */
#define SD_CARD_DRIVER SD_Driver_DMA

/* Si5351 linkage */
#define Si5351_I2C_HANDLE hi2c2

/* SDIO handler linkage */
#define SDIO_HANDLE hsd

/* ADC Handlers */
#define ANALOG_KEYBOARD_ADC_HANDLER hadc3
#define BATTERY_VOLTAGE_ADC_HANDLER hadc3

/* DSP task timer linkage */
#define TASKS_TIMER_TYPEDEF TIM14
#define TASKS_TIMER_HANDLE htim14
#define TASKS_TIMER_TYPEDEF_CLOCK_HZ (SystemCoreClock / APB1_PRESCALER << 1) // TIM14 is on the APB1 BUS

/* LED blink timer linkage */
#define LED_TIMER_TYPEDEF TIM3
#define LED_TIMER_HANDLE htim3
#define LED_TIMER_TYPEDEF_CLOCK_HZ (SystemCoreClock / APB1_PRESCALER << 1) // TIM14 is on the APB1 BUS

/* LCD SPI Linkage */
#define LCD_SPI_HANDLE hspi2 // link the LCD SPI to hspi2 instance, defined in the includes

/* Input pin controller timer linkage */
#define INPUT_PIN_CONTROLLER_TIMER TIM13
#define INPUT_PIN_CONTROLLER_TIMER_CLOCK_HZ (SystemCoreClock / APB1_PRESCALER << 1) // TIM13 is on the APB1 BUS

#define ADC_DMA_TIMER TIM2
#define ADC_DMA_TIMER_CLOCK_HZ (SystemCoreClock / APB1_PRESCALER << 1) // TIM2 is on the APB1 BUS

#define DAC_TIMER TIM6
#define DAC_TIMER_CLOCK_HZ (SystemCoreClock / APB1_PRESCALER << 1) // TIM6 is on the APB1 BUS

#else

#define HSE_VALUE ((uint32_t)16000000) /*!< Value of the External oscillator in Hz */
#define CPU_CORE_FREQUENCY_HZ SystemCoreClock
#define CPU_TIMER_PRESCALER_HZ_MS 72000
/* FATFS handle linkage */
#define SD_CARD_DRIVER SPI_Driver
#define SD_SPI_HANDLE hspi1 // link the SD SPI to hspi1 instance, defined in the includes

#include <stm32f3xx.h>
#include "stm32f3xx_hal_dac_ex.h"
#include "hw/stm32f3xx/dac.h"
#include "hw/stm32f3xx/dma.h"
#include "hw/stm32f3xx/adc.h"
#include "hw/stm32f3xx/timers.h"
#include "hw/stm32f3xx/flash.h"
#include "hw/stm32f3xx/clocks.h"
#include "hw/stm32f3xx/connectivity.h"
#include "hw/stm32f3xx/gpio.h"

/* DSP task timer linkage */
#define TASKS_TIMER_TYPEDEF TIM15
#define TASKS_TIMER_HANDLE htim15
#define TASKS_TIMER_TYPEDEF_CLOCK_HZ SystemCoreClock

/* LCD SPI Linkage */
#define LCD_SPI_HANDLE hspi1 // link the LCD SPI to hspi1 instance, defined in the includes

/* Input pin controller timer linkage */
#define INPUT_PIN_CONTROLLER_TIMER TIM2
#define INPUT_PIN_CONTROLLER_TIMER_CLOCK_HZ SystemCoreClock

/* ADC Handlers */
#define ANALOG_KEYBOARD_ADC_HANDLER hadc1

#define ADC_DMA_TIMER TIM4
#define ADC_DMA_TIMER_CLOCK_HZ SystemCoreClock

#define DAC_TIMER TIM6
#define DAC_TIMER_CLOCK_HZ SystemCoreClock // TIM6 is on the APB1 BUS

#endif

#endif // TRX_FRONTEND_STM32_H
