// stm32f4xx.h - Mock header for Linux build
#ifndef STM32F4XX_H
#define STM32F4XX_H

#include <stdint.h>

// Mock STM32 types and defines
typedef uint32_t GPIO_PinState;
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0

// Mock HAL functions
extern "C" {
uint32_t HAL_GetTick(void);
int32_t __SSAT(int32_t val, int32_t sat);
}

// Other common STM32 defines you might need
#define __SIMD32(addr) (*(int32_t **)&(addr))
#define __SMUAD(x, y) ((int32_t)(((short)(x) * (short)(y)) + (((short)((x) >> 16)) * ((short)((y) >> 16)))))

#endif
