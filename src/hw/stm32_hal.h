#pragma once

#ifndef TRX_FRONTEND_STM32_HAL_H
#define TRX_FRONTEND_STM32_HAL_H

#ifndef STM32F4xx
#define STM32F4xx
#endif

#ifdef STM32F4xx

#define DISABLE_SD_INIT 1
#include <stm32f4xx.h>

#else

#include <stm32f3xx.h>

#endif

#endif //TRX_FRONTEND_STM32_HAL_H
