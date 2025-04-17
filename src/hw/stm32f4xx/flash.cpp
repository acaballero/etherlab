//
// Created by Angel Dust on 20/05/2021.
//

#include "flash.h"
#include "eeprom.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_flash.h"
#include <string>

/*
uint8_t flash_write(uint32_t *src, uint32_t size) {

    // The commented lines are for performing a CRC error check using the CRC peripheral of the STM32

    // GLOBAL_settings_ptr->revision++;

    // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);

    // CRC_ResetDR();

    // GLOBAL_settings_ptr->crc32 = CRC_CalcBlockCRC( //calculate new CRC
    //         (uint32_t *)GLOBAL_settings_ptr,
    //         (sizeof(Config)-sizeof(uint32_t))/sizeof(uint32_t)//size minus the crc32 at the end, IN WORDS
    //);

    // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, DISABLE);

    HAL_FLASH_Unlock();

    uint32_t *dst = (uint32_t *)DATA_EEPROM_START_ADDR;
    uint8_t ret = HAL_FLASH_ERROR_NONE;

    //------------------ bug of the day ------------------
    // Flash option byte user validity error flag caused write to EEPROM operation to fail!
    //  * The problem did not happen when J-Link was connected. Probably J-Link starts execution
    //  * right from the application address, skipping the bootloader. The bootloader
    //  * may have had set the flag. //
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    //------------------ bug of the day ------------------

    // write settings word (uint32_t) at a time

    // A flash memory bit can only be changed from 1 to 0 so, in order to write, we should erase the page before (set every bit to 1)
    // Otherwise, the HAL_FLASH_Program function will return a FLASH_FLAG_PGERR
    FLASH_EraseInitTypeDef flashEraseDef;

    flashEraseDef.NbSectors = NFLASHPAGES;
    flashEraseDef.Sector = GetSector(DATA_EEPROM_START_ADDR);
    flashEraseDef.TypeErase = FLASH_TYPEERASE_SECTORS;
    flashEraseDef.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    uint32_t err = HAL_FLASH_ERROR_NONE;

    HAL_FLASHEx_Erase(&flashEraseDef, &err);

    for (uint32_t i = 0; i < size; i++) {

        //__DSB();
    // uint32_t existing = *(volatile uint32_t *)(dst + i);
    // if (existing != src[i]) { // Write only if different
            ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uintptr_t)(dst + i), src[i]);

            if (ret != HAL_FLASH_ERROR_NONE) {

                break;
            }
        //}
    }

    HAL_FLASH_Lock();
    return ret;
}
*/

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
