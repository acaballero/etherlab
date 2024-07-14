//
// Created by Angel Dust on 20/05/2021.
//

#include "flash.h"



/**
  * @brief  Gets the sector of a given address
  * @param  None
  * @retval The sector of a given address
  */
static uint32_t GetSector(uint32_t Address) {
    uint32_t sector = 0;

    if ((Address < 0x08003FFF) && (Address >= 0x08000000)) {
        sector = FLASH_SECTOR_0;
    } else if ((Address < 0x08007FFF) && (Address >= 0x08004000)) {
        sector = FLASH_SECTOR_1;
    } else if ((Address < 0x0800BFFF) && (Address >= 0x08008000)) {
        sector = FLASH_SECTOR_2;
    } else if ((Address < 0x0800FFFF) && (Address >= 0x0800C000)) {
        sector = FLASH_SECTOR_3;
    } else if ((Address < 0x0801FFFF) && (Address >= 0x08010000)) {
        sector = FLASH_SECTOR_4;
    } else if ((Address < 0x0803FFFF) && (Address >= 0x08020000)) {
        sector = FLASH_SECTOR_5;
    } else if ((Address < 0x0805FFFF) && (Address >= 0x08040000)) {
        sector = FLASH_SECTOR_6;
    } else if ((Address < 0x0807FFFF) && (Address >= 0x08060000)) {
        sector = FLASH_SECTOR_7;
    } else if ((Address < 0x0809FFFF) && (Address >= 0x08080000)) {
        sector = FLASH_SECTOR_8;
    } else if ((Address < 0x080BFFFF) && (Address >= 0x080A0000)) {
        sector = FLASH_SECTOR_9;
    } else if ((Address < 0x080DFFFF) && (Address >= 0x080C0000)) {
        sector = FLASH_SECTOR_10;
    } else if ((Address < 0x080FFFFF) && (Address >= 0x080E0000)) {
        sector = FLASH_SECTOR_11;
    } else if ((Address < 0x08103FFF) && (Address >= 0x08100000)) {
        sector = FLASH_SECTOR_12;
    } else if ((Address < 0x08107FFF) && (Address >= 0x08104000)) {
        sector = FLASH_SECTOR_13;
    } else if ((Address < 0x0810BFFF) && (Address >= 0x08108000)) {
        sector = FLASH_SECTOR_14;
    } else if ((Address < 0x0810FFFF) && (Address >= 0x0810C000)) {
        sector = FLASH_SECTOR_15;
    } else if ((Address < 0x0811FFFF) && (Address >= 0x08110000)) {
        sector = FLASH_SECTOR_16;
    } else if ((Address < 0x0813FFFF) && (Address >= 0x08120000)) {
        sector = FLASH_SECTOR_17;
    } else if ((Address < 0x0815FFFF) && (Address >= 0x08140000)) {
        sector = FLASH_SECTOR_18;
    } else if ((Address < 0x0817FFFF) && (Address >= 0x08160000)) {
        sector = FLASH_SECTOR_19;
    } else if ((Address < 0x0819FFFF) && (Address >= 0x08180000)) {
        sector = FLASH_SECTOR_20;
    } else if ((Address < 0x081BFFFF) && (Address >= 0x081A0000)) {
        sector = FLASH_SECTOR_21;
    } else if ((Address < 0x081DFFFF) && (Address >= 0x081C0000)) {
        sector = FLASH_SECTOR_22;
    } else if ((Address < 0x081FFFFF) && (Address >= 0x081E0000))
    {
        sector = FLASH_SECTOR_23;
    }
    return sector;
}

uint8_t flash_write(uint32_t *src, uint32_t size) {

    // The commented lines are for performing a CRC error check using the CRC peripheral of the STM32

    //GLOBAL_settings_ptr->revision++;

    //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);

    //CRC_ResetDR();

    //GLOBAL_settings_ptr->crc32 = CRC_CalcBlockCRC( //calculate new CRC
    //        (uint32_t *)GLOBAL_settings_ptr,
    //        (sizeof(Config)-sizeof(uint32_t))/sizeof(uint32_t)/*size minus the crc32 at the end, IN WORDS*/
    //);

    //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, DISABLE);

    HAL_FLASH_Unlock();


    uint32_t *dst = (uint32_t *) DATA_EEPROM_START_ADDR;
    uint8_t ret = HAL_FLASH_ERROR_NONE;


    //------------------ bug of the day ------------------
    /* Flash option byte user validity error flag caused write to EEPROM operation to fail!
     * The problem did not happen when J-Link was connected. Probably J-Link starts execution
     * right from the application address, skipping the bootloader. The bootloader
     * may have had set the flag.
     */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP |
                           FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    //------------------ bug of the day ------------------

    //write settings word (uint32_t) at a time

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

        ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t) dst, *src);

        if (ret != HAL_FLASH_ERROR_NONE) {

            break;

        }
        src++;
        dst++;
    }

    HAL_FLASH_Lock();
    return ret;

}


