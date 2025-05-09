//
// Created by Angel Dust on 20/05/2021.
//

#include "flash.h"
#include "eeprom.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_flash.h"
#include <string>

uint8_t flash_write(uint16_t *src, uint32_t size) {

    HAL_FLASH_Unlock();

    uint8_t ret = HAL_FLASH_ERROR_NONE;

    //------------------ bug of the day ------------------
    /* Flash option byte user validity error flag caused write to EEPROM operation to fail!
     * The problem did not happen when J-Link was connected. Probably J-Link starts execution
     * right from the application address, skipping the bootloader. The bootloader
     * may have had set the flag.
     */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    //------------------ bug of the day ------------------

    uint16_t startAddress = 1;
    uint16_t old;

    for (uint32_t i = 0; i < size; i++) {

        EE_Status ret = EE_ReadVariable(startAddress + i, &old);

        if (ret != EE_OK && ret != EE_NO_DATA) {
            return HAL_FLASH_ERROR_OPERATION;
        }

        if (ret == EE_NO_DATA || old != src[i]) {
            if (EE_WriteVariable(startAddress + i, src[i]) != EE_OK) {
                return HAL_FLASH_ERROR_OPERATION;
            }
        }
    }

    HAL_FLASH_Lock();
    return ret;
}

uint8_t flash_read(uint16_t *dst, uint32_t size) {

    HAL_FLASH_Unlock();

    uint8_t ret = HAL_FLASH_ERROR_NONE;

    //------------------ bug of the day ------------------
    /* Flash option byte user validity error flag caused write to EEPROM operation to fail!
     * The problem did not happen when J-Link was connected. Probably J-Link starts execution
     * right from the application address, skipping the bootloader. The bootloader
     * may have had set the flag.
     */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    //------------------ bug of the day ------------------

    uint16_t startAddress = 1;

    for (uint32_t i = 0; i < size; i++) {

        if (EE_ReadVariable(startAddress + i, &dst[i]) != EE_OK) {
            return EE_NO_DATA;
        }
    }

    HAL_FLASH_Lock();
    return ret;
}
