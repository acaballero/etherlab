/**
 ******************************************************************************
 * @file    EEPROM_Emulation/src/eeprom.c
 * @author  MCD Application Team
 * @version V1.5.0
 * @date    29-April-2016
 * @brief   This file provides all the EEPROM emulation firmware functions.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics International N.V.
 * All rights reserved.</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions are met:
 *
 * 1. Redistribution of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of other
 *    contributors to this software may be used to endorse or promote products
 *    derived from this software without specific written permission.
 * 4. This software, including modifications and/or derivative works of this
 *    software, must execute solely and exclusively on microcontroller or
 *    microprocessor devices manufactured by or for STMicroelectronics.
 * 5. Redistribution and use of this software other than as permitted under
 *    this license is void and will automatically terminate your rights under
 *    this license.
 *
 * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
 * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
 * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/** @addtogroup EEPROM_Emulation
 * @{
 */

/* Includes ------------------------------------------------------------------*/
#include "hw/stm32.h"
#include "eeprom.h"
#include "stm32f4xx_hal_flash.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/*
   This definition part is used to describe the page setting and
   can be custom
*/

#define PAGE0_NUMBER (uint32_t)10

#define PAGE1_NUMBER (uint32_t)11

/*
   End of the page definition
*/

/* Page stat header */
#define EE_PAGESTAT_ERASED ((uint32_t)0xFFFFFFFF)
#define EE_PAGESTAT_VALID ((uint32_t)0x00000000)
#define EE_PAGESTAT_RECEIVE ((uint32_t)0x0000AAAA)

#define EE_DATA_SIZE 4
#define EE_DATA_TYPE uint32_t
#define EE_DATA_SHIFT 0
#define EE_MASK_VIRTUALADRESS (uint64_t)0x0000FFFF
#define EE_MASK_DATA (uint32_t)0xFFFF0000
#define EE_MASK_FULL (uint32_t)0xFFFFFFFF

/* Type of find requested :
       READ -> page in valid state
       WRTIE --> page in recpetion state or valid state
       ERASE --> page in erase state */
typedef enum { FIND_READ_PAGE, FIND_WRITE_PAGE, FIND_ERASE_PAGE } EE_Find_type;

/* No valid page define */
#define EE_NO_VALID_PAGE ((uint32_t)0xFFFFFFFF)

/* defintion of the different type of page transfer
        NORMAL  -> copie data pag source to page destination
        RECOVER -> resolve confict when one page reception and a second is valid */
typedef enum { EE_TRANSFER_NORMAL, EE_TRANSFER_RECOVER } EE_Transfer_type;

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variable used to store variable value in read sequence */

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static EE_Status EE_Format(void);
static uint32_t EE_FindPage(EE_Find_type Operation);
static EE_Status EE_VerifyPageFullWriteVariable(EE_VIRTUALADDRESS_TYPE VirtAddress, EE_DATA_STORED_TYPE Data);
static EE_Status EE_PageTransfer(EE_VIRTUALADDRESS_TYPE VirtAddress, EE_DATA_STORED_TYPE Data, EE_Transfer_type type);
static EE_Status EE_VerifyPageFullyErased(uint32_t Address, uint32_t PageSize);
static EE_Status EE_PageErase(uint32_t Page);

/**
 * @brief  Restore the pages to a known good state in case of page's status
 *   corruption after a power loss.
 * @param  None.
 * @retval - EE_OK in cas of succes
 *         - EE error code in case of error
 */
EE_Status EE_Init(void) {
    EE_DATA_TYPE pagestatus0, pagestatus1, addressvalue;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    /* Get Page0 status */
    pagestatus0 = (*(__IO EE_DATA_TYPE *)PAGE0_BASE_ADDRESS);
    /* Get Page1 status */
    pagestatus1 = (*(__IO EE_DATA_TYPE *)PAGE1_BASE_ADDRESS);

    /* Check for invalid header states and repair if necessary */
    switch (pagestatus0) {
        case EE_PAGESTAT_ERASED: {
            if (pagestatus1 == EE_PAGESTAT_VALID) /* Page0 erased, Page1 valid */
            {
                /* Erase Page0 */
                if (EE_VerifyPageFullyErased(PAGE0_BASE_ADDRESS, PAGE_SIZE) == EE_PAGE_NOTERASED) {
                    if (EE_PageErase(PAGE0_NUMBER) != EE_OK) {
                        return EE_ERASE_ERROR;
                    }
                }
            } else if (pagestatus1 == EE_PAGESTAT_RECEIVE) /* Page0 erased, Page1 receive */
            {
                /* Erase Page0 */
                if (EE_VerifyPageFullyErased(PAGE0_BASE_ADDRESS, PAGE_SIZE) == EE_PAGE_NOTERASED) {
                    if (EE_PageErase(PAGE0_NUMBER) != EE_OK) {
                        return EE_ERASE_ERROR;
                    }
                }

                /* Mark Page1 as valid */
                /* If program operation was failed, a Flash error code is returned */
                if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, PAGE1_BASE_ADDRESS, EE_PAGESTAT_VALID) != HAL_OK) {
                    return EE_WRITE_ERROR;
                }
            } else /* First EEPROM access (Page0&1 are erased) or invalid state -> format EEPROM */
            {
                /* Erase both Page0 and Page1 and set Page0 as valid page */
                /* If erase/program operation was failed, a Flash error code is returned */
                if (EE_Format() != EE_OK) {
                    return EE_FORMAT_ERROR;
                }
            }
        } break;

        case EE_PAGESTAT_RECEIVE: {
            if (pagestatus1 == EE_PAGESTAT_VALID) /* Page0 receive, Page1 valid */
            {
                /* Get the first udpated value from the reception page */
                addressvalue = (*(__IO EE_DATA_TYPE *)(PAGE0_BASE_ADDRESS + EE_DATA_SIZE));
                /* Restart the interrupted page transfer */
                if (EE_PageTransfer((addressvalue & EE_MASK_VIRTUALADRESS) >> EE_DATA_SHIFT,
                                    (EE_DATA_STORED_TYPE)((addressvalue & EE_MASK_DATA) >> (EE_DATA_SHIFT + 16)), EE_TRANSFER_RECOVER) != EE_OK) {
                    return EE_TRANSFER_ERROR;
                }
            } else if (pagestatus1 == EE_PAGESTAT_ERASED) /* Page0 receive, Page1 erased */
            {
                /* Erase Page1 */
                if (EE_VerifyPageFullyErased(PAGE1_BASE_ADDRESS, PAGE_SIZE) == EE_PAGE_NOTERASED) {
                    /* If erase operation was failed, a Flash error code is returned */
                    if (EE_PageErase(PAGE1_NUMBER) != EE_OK) {
                        return EE_ERASE_ERROR;
                    }
                }
                /* Mark Page0 as valid */
                /* If program operation was failed, a Flash error code is returned */
                if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, PAGE0_BASE_ADDRESS, EE_PAGESTAT_VALID) != HAL_OK) {
                    return EE_WRITE_ERROR;
                }
            } else /* Invalid state -> format eeprom */
            {
                /* Erase both Page0 and Page1 and set Page0 as valid page */
                /* If erase/program operation was failed, a Flash error code is returned */
                if (EE_Format() != EE_OK) {
                    return EE_FORMAT_ERROR;
                }
            }
        } break;

        case EE_PAGESTAT_VALID: {
            if (pagestatus1 == EE_PAGESTAT_VALID) /* Invalid state -> format eeprom */
            {
                /* Erase both Page0 and Page1 and set Page0 as valid page */
                /* If erase/program operation was failed, a Flash error code is returned */
                if (EE_Format() != EE_OK) {
                    return EE_FORMAT_ERROR;
                }
            } else if (pagestatus1 == EE_PAGESTAT_ERASED) /* Page0 valid, Page1 erased */
            {
                /* Erase Page1 */
                if (EE_VerifyPageFullyErased(PAGE1_BASE_ADDRESS, PAGE_SIZE) == EE_PAGE_NOTERASED) {
                    /* If erase operation was failed, a Flash error code is returned */
                    if (EE_PageErase(PAGE1_NUMBER) != EE_OK) {
                        return EE_ERASE_ERROR;
                    }
                }
            } else /* Page0 valid, Page1 receive */
            {
                /* Get the first udpated value from the reception page */
                addressvalue = (*(__IO EE_DATA_TYPE *)(PAGE1_BASE_ADDRESS + EE_DATA_SIZE));
                /* Restart the interrupted page transfer */
                if (EE_PageTransfer((addressvalue & EE_MASK_VIRTUALADRESS) >> EE_DATA_SHIFT,
                                    (EE_DATA_STORED_TYPE)((addressvalue & EE_MASK_DATA) >> (EE_DATA_SHIFT + 16)), EE_TRANSFER_RECOVER) != EE_OK) {
                    return EE_TRANSFER_ERROR;
                }
            }
        } break;

        default: /* Any other state -> format eeprom */
        {
            /* Erase both Page0 and Page1 and set Page0 as valid page */
            /* If erase/program operation was failed, a Flash error code is returned */
            if (EE_Format() != EE_OK) {
                return EE_FORMAT_ERROR;
            }
        } break;
    }

    return EE_OK;
}

/**
 * @brief  Verify if specified page is fully erased.
 * @param  Address: page address
 *   This parameter can be one of the following values:
 *     @arg PAGE0_BASE_ADDRESS: Page0 base address
 *     @arg PAGE1_BASE_ADDRESS: Page1 base address
 * @param  PageSize: page size
 * @retval page fully erased status:
 *           - EE_PAGE_NOTERASED : if Page not erased
 *           - EE_PAGE_ERASED    : if Page erased
 */
EE_Status EE_VerifyPageFullyErased(uint32_t Address, uint32_t PageSize) {
    EE_Status readstatus = EE_PAGE_ERASED;
    uint64_t counter = 0;

    /* Check each active page address starting from end */
    while (counter < PageSize) {
        /* Compare the read address with the virtual address */
        if ((*(__IO EE_DATA_TYPE *)(Address + counter)) != EE_PAGESTAT_ERASED) {
            /* In case variable value is read, reset readstatus flag */
            readstatus = EE_PAGE_NOTERASED;
            break;
        }
        /* Next address location */
        counter = counter + EE_DATA_SIZE;
    }

    /* Return readstatus value: (0: Page not erased, 1: Page erased) */
    return readstatus;
}

/**
 * @brief  Returns the last stored variable data, if found, which correspond to
 *   the passed virtual address
 * @param  VirtAddress: Variable virtual address
 * @param  Data: Global variable contains the read variable value
 * @retval Success or error status:
 *           - EE_OK: if variable was found
 *           - EE_NO_DATA: if the variable was not found
 *           - EE_ERROR_NOVALID_PAGE: if no valid page was found.
 */
EE_Status EE_ReadVariable(EE_VIRTUALADDRESS_TYPE VirtAddress, EE_DATA_STORED_TYPE *Data) {
    EE_DATA_TYPE addressvalue;
    uint32_t counter = PAGE_SIZE - EE_DATA_SIZE;

    /* Get active Page for read operation */
    uint32_t validpageadresse = EE_FindPage(FIND_READ_PAGE);

    /* Check if there is no valid page */
    if (validpageadresse == EE_NO_VALID_PAGE) {
        return EE_ERROR_NOVALID_PAGE;
    }

    /* Check each active page address starting from end */
    while (counter >= EE_DATA_SIZE) {
        /* Get the current location content to be compared with virtual address */
        addressvalue = (*(__IO EE_DATA_TYPE *)(validpageadresse + counter));
        if (addressvalue != EE_PAGESTAT_ERASED) {
            /* Compare the read address with the virtual address */
            if ((addressvalue & EE_MASK_VIRTUALADRESS) == ((EE_DATA_TYPE)VirtAddress << EE_DATA_SHIFT)) {
                /* Get content of Address-2 which is variable value */
                *Data = (EE_DATA_STORED_TYPE)((addressvalue & EE_MASK_DATA) >> (EE_DATA_SHIFT + 16));
                /* In case variable value is read, reset readstatus flag */
                return EE_OK;
            }
        }
        /* Next address location */
        counter -= EE_DATA_SIZE;
    }

    /* Return readstatus value: (EE_OK: variable exist, EE_ERROR: variable doesn't exist) */
    return EE_NO_DATA;
}

/**
 * @brief  Writes/upadtes variable data in EEPROM.
 * @param  VirtAddress: Variable virtual address
 * @param  Data: 16 bit data to be written
 * @retval Success or error status:
 *           - EE_OK: on success
 *           - EE error code: if an error occurs
 */
EE_Status EE_WriteVariable(EE_VIRTUALADDRESS_TYPE VirtAddress, EE_DATA_STORED_TYPE Data) {
    EE_Status status;

    /* Write the variable virtual address and value in the EEPROM */
    status = EE_VerifyPageFullWriteVariable(VirtAddress, Data);
    if (status == EE_PAGE_FULL) {
        /* In case the EEPROM active page is full */
        /* Perform Page transfer */
        return EE_PageTransfer(VirtAddress, Data, EE_TRANSFER_NORMAL);
    }

    /* Return last operation status */
    return status;
}

/*
 * @brief  Erases PAGE and PAGE1 and writes VALID_PAGE header to PAGE
 * @param  None
 * @retval Success or error status:
 *           - EE_OK: on success
 *           - EE error code: if an error occurs
 */
static EE_Status EE_Format(void) {
    /* Erase Page0 */
    if (EE_VerifyPageFullyErased(PAGE0_BASE_ADDRESS, PAGE_SIZE) == EE_PAGE_NOTERASED) {
        if (EE_PageErase(PAGE0_NUMBER) != EE_OK) {
            return EE_ERASE_ERROR;
        }
    }

    /* If program operation was failed, a Flash error code is returned */
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, PAGE0_BASE_ADDRESS, EE_PAGESTAT_VALID) != HAL_OK) {
        return EE_WRITE_ERROR;
    }

    /* Erase Page1 */
    if (EE_VerifyPageFullyErased(PAGE1_BASE_ADDRESS, PAGE_SIZE) == EE_PAGE_NOTERASED) {
        if (EE_PageErase(PAGE1_NUMBER) != EE_OK) {
            return EE_ERASE_ERROR;
        }
    }

    return EE_OK;
}

/**
 * @brief  Find Page
 * @param  type: type of page to requested.
 *   This parameter can be one of the following values:
 *     @arg FIND_READ_PAGE: return the read page address
 *     @arg FIND_WRITE_PAGE: return the write page address
 *     @arg FIND_ERASE_PAGE: return the erase page address
 * @retval :
 *           - Page @: on success
 *           - EE_NO_VALID_PAGE : if an error occurs
 */
static uint32_t EE_FindPage(EE_Find_type Operation) {
    EE_DATA_TYPE pagestatus0, pagestatus1;

    /* Get Page0 actual status */
    pagestatus0 = (*(__IO EE_DATA_TYPE *)PAGE0_BASE_ADDRESS);

    /* Get Page1 actual status */
    pagestatus1 = (*(__IO EE_DATA_TYPE *)PAGE1_BASE_ADDRESS);

    /* Write or read operation */
    if (Operation == FIND_WRITE_PAGE) {
        /* ---- Write operation ---- */
        if (pagestatus1 == EE_PAGESTAT_VALID) {
            /* Page0 receiving data */
            if (pagestatus0 == EE_PAGESTAT_RECEIVE) {
                return PAGE0_BASE_ADDRESS; /* Page0 valid */
            } else {
                return PAGE1_BASE_ADDRESS; /* Page1 valid */
            }
        } else if (pagestatus0 == EE_PAGESTAT_VALID) {
            /* Page1 receiving data */
            if (pagestatus1 == EE_PAGESTAT_RECEIVE) {
                return PAGE1_BASE_ADDRESS; /* Page1 valid */
            } else {
                return PAGE0_BASE_ADDRESS; /* Page0 valid */
            }
        } else {
            return EE_NO_VALID_PAGE; /* No valid Page */
        }
    } else if (Operation == FIND_READ_PAGE) {
        /* ---- Read operation ---- */
        if (pagestatus0 == EE_PAGESTAT_VALID) {
            return PAGE0_BASE_ADDRESS; /* Page0 valid */
        } else if (pagestatus1 == EE_PAGESTAT_VALID) {
            return PAGE1_BASE_ADDRESS; /* Page1 valid */
        } else {
            return EE_NO_VALID_PAGE; /* No valid Page */
        }
    } else if (Operation == FIND_ERASE_PAGE) {
        /* ---- Return the erased page */
        if (pagestatus0 == EE_PAGESTAT_ERASED) {
            return PAGE0_BASE_ADDRESS;
        }
        if (pagestatus1 == EE_PAGESTAT_ERASED) {
            return PAGE1_BASE_ADDRESS;
        }
    } else {
        return EE_NO_VALID_PAGE;
    }

    return EE_NO_VALID_PAGE;
}

/**
 * @brief  Verify if active page is full and Writes variable in EEPROM.
 * @param  VirtAddress: 16 bit virtual address of the variable
 * @param  Data: 16 bit data to be written as variable value
 * @retval Success or error status:
 *           - EE_OK: on success
 *           - EE_FULL: if the page is full
 *           - EE error code: if an error occurs
 */
static EE_Status EE_VerifyPageFullWriteVariable(EE_VIRTUALADDRESS_TYPE VirtAddress, EE_DATA_STORED_TYPE Data) {
    uint32_t count = EE_DATA_SIZE; /* start the check after the header */

    /* Get valid Page for write operation */
    uint32_t validpage = EE_FindPage(FIND_WRITE_PAGE);
    /* Check if there is no valid page */
    if (validpage == EE_NO_VALID_PAGE) {
        return EE_ERROR_NOVALID_PAGE;
    }

    /* Check each active page address starting from begining */
    while (count < PAGE_SIZE) {
        EE_DATA_TYPE value = (*(__IO EE_DATA_TYPE *)(validpage + count));
        /* Verify if address contents is erased */
        if (value == EE_MASK_FULL) {
            /* Set variable data + virtual adress */
            /* If program operation was failed, a Flash error code is returned */
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, validpage + count, ((((EE_DATA_TYPE)Data) << 16) | VirtAddress) << EE_DATA_SHIFT) != HAL_OK) {
                return EE_WRITE_ERROR;
            }
            return EE_OK;
        } else {
            /* Next address location */
            count += EE_DATA_SIZE;
        }
    }
    /* Return PAGE_FULL in case the valid page is full */
    return EE_PAGE_FULL;
}

/**
 * @brief  Transfers last updated variables data from the full Page to
 *   an empty one.
 * @param  VirtAddress: 16 bit virtual address of the variable
 * @param  Data: 16 bit data to be written as variable value
 * @retval Success or error status:
 *           - EE_OK: on success
 *           - EE error code: if an error occurs
 */
static EE_Status EE_PageTransfer(EE_VIRTUALADDRESS_TYPE VirtAddress, EE_DATA_STORED_TYPE Data, EE_Transfer_type type) {
    uint32_t activepageaddress, newpageaddress;
    uint32_t varidx = 0;
    EE_DATA_STORED_TYPE DataValue;

    /* Get active Page for read operation */
    activepageaddress = EE_FindPage(FIND_READ_PAGE);
    if (activepageaddress == EE_NO_VALID_PAGE) {
        return EE_ERROR_NOVALID_PAGE;
    }

    /* Get active Page for read operation */
    newpageaddress = EE_FindPage((type == EE_TRANSFER_NORMAL ? FIND_ERASE_PAGE : FIND_WRITE_PAGE));
    if (newpageaddress == EE_NO_VALID_PAGE) {
        return EE_ERROR_NOVALID_PAGE;
    }

    /* Mark the ativepage at receive state */
    /* If program operation was failed, a Flash error code is returned */
    if (type == EE_TRANSFER_NORMAL) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, newpageaddress, EE_PAGESTAT_RECEIVE) != HAL_OK) {
            return EE_WRITE_ERROR;
        }
    }

    /* Write the variable passed as parameter in the new active page */
    /* If program operation was failed, a Flash error code is returned */
    if (EE_VerifyPageFullWriteVariable(VirtAddress, Data) != EE_OK) {
        return EE_WRITE_ERROR;
    }

    /* Transfer process: transfer variables from old to the new active page */
    for (varidx = 0; varidx < NB_OF_VAR; varidx++) {
        if (varidx != VirtAddress) /* Check each variable except the one passed as parameter */
        {
            /* Read the other last variable updates */
            if (EE_ReadVariable(varidx, &DataValue) == EE_OK) {
                /* In case variable corresponding to the virtual address was found */
                /* Transfer the variable to the new active page */
                /* If program operation was failed, a Flash error code is returned */
                if (EE_VerifyPageFullWriteVariable(varidx, DataValue) != EE_OK) {
                    return EE_WRITE_ERROR;
                }
            }
        }
    }

    /* Erase the current VALID_PAGE */
    if (EE_PageErase(EE_GetPageNumber(activepageaddress)) != EE_OK) {
        return EE_ERASE_ERROR;
    }

    /* Set new Page status to VALID_PAGE status */
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, newpageaddress, EE_PAGESTAT_VALID) != HAL_OK) {
        return EE_WRITE_ERROR;
    }

    /* Return last operation flash status */
    return EE_OK;
}

/**
 * @brief  Erase a page.
 * @param  Page: 32 bit Page number
 * @param  BankNb: 32 bit Bank number
 * @param  Data: 16 bit data to be written as variable value
 * @retval Success or error status:
 *           - EE_OK: on success
 *           - EE error code: if an error occurs
 */
static EE_Status EE_PageErase(uint32_t Page) {
    FLASH_EraseInitTypeDef s_eraseinit;
    uint32_t page_error;

    s_eraseinit.TypeErase = FLASH_TYPEERASE_SECTORS;
    s_eraseinit.NbSectors = 1;
    s_eraseinit.Sector = Page;
    s_eraseinit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    /* Erase the old Page: Set old Page status to ERASED status */
    if (HAL_FLASHEx_Erase(&s_eraseinit, &page_error) != HAL_OK) {
        return EE_ERASE_ERROR;
    }
    return EE_OK;
}

/**
 * @brief  Gets the page of a given address
 * @param  Address: Address of the FLASH Memory
 * @retval The page of a given address
 */
static uint32_t EE_GetPageNumber(uint32_t Address) {
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
    } else if ((Address < 0x081FFFFF) && (Address >= 0x081E0000)) {
        sector = FLASH_SECTOR_23;
    }
    return sector;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
