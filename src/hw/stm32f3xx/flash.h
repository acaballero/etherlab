//
// Created by Angel Dust on 20/05/2021.
//

#ifndef TRX_FRONTEND_FLASH_H
#define TRX_FRONTEND_FLASH_H

/* Start address of the settings in flash memory space. On STM32f303, the flash starts at 0x080000000, has 128 pages of 2kb each,  so
 * here we will use 0x080300000 (Leaving the first 192kb for program .text space)
 * Be aware that if the entire FLASH is erased when the program is flashed, the settings will be lost,
 * so we should instruct the linker not to use FLASH space after DATA_EEPROM_START_ADDR */
#define NFLASHPAGES 2
//#define DATA_EEPROM_START_ADDR     0x0803f800 // PAGE 128-NFLASHPAGES
#define DATA_EEPROM_START_ADDR (0x8040000-(0x800*NFLASHPAGES))
#define DATA_EEPROM_PAGE           127
#define DATA_EEPROM_SIZE_BYTES     2048

uint8_t flash_write(uint32_t *src,uint32_t size);

#endif //TRX_FRONTEND_FLASH_H
