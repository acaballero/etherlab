//
// Created by Angel Dust on 20/05/2021.
//

#ifndef TRX_FRONTEND_FLASH_H
#define TRX_FRONTEND_FLASH_H

#include "hw/stm32.h"

/* Start address of the settings in flash memory space. On STM32f427, the flash starts at 0x080000000, has (with single bank enabled))
 * 11 sectors of different sizes. The last one is 128k.
 * here we will use  0x080E0000 (Leaving the first 192kb for program .text space)
 * Be aware that if the entire FLASH is erased when the program is flashed, the settings will be lost,
 * so we should instruct the linker not to use FLASH space after DATA_EEPROM_START_ADDR */
#define NFLASHPAGES 1
#define DATA_EEPROM_START_ADDR (0x080E0000)
#define DATA_EEPROM_SIZE_BYTES     131072

uint8_t flash_write(uint32_t *src,uint32_t size);


#endif //TRX_FRONTEND_FLASH_H
