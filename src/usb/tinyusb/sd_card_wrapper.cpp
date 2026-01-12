/**
 * sd_card_wrapper.c
 *
 * Wrapper functions to connect your existing SD card code to TinyUSB MSC
 */

#include "usb_composite_device.h"
#include "fatfs/fatfs.h"
#include "stm32f4xx_hal.h"

// External SD card handle from your code
extern SD_HandleTypeDef hsd;

/**
 * Read blocks from SD card
 * @param lba Logical block address
 * @param buffer Buffer to store data
 * @param block_count Number of 512-byte blocks to read
 * @return 0 on success, -1 on error
 */
int sd_card_read_blocks(uint32_t lba, uint8_t *buffer, uint32_t block_count) {
    HAL_StatusTypeDef status;

    // Use HAL to read blocks
    status = HAL_SD_ReadBlocks(&hsd, buffer, lba, block_count, 1000);

    if (status != HAL_OK) {
        return -1;
    }

    // Wait for transfer to complete if using DMA
    // If you're using DMA, uncomment this:
    // while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {}

    return 0;
}

/**
 * Write blocks to SD card
 * @param lba Logical block address
 * @param buffer Data to write
 * @param block_count Number of 512-byte blocks to write
 * @return 0 on success, -1 on error
 */
int sd_card_write_blocks(uint32_t lba, const uint8_t *buffer, uint32_t block_count) {
    HAL_StatusTypeDef status;

    // Use HAL to write blocks
    status = HAL_SD_WriteBlocks(&hsd, (uint8_t *)buffer, lba, block_count, 1000);

    if (status != HAL_OK) {
        return -1;
    }

    // Wait for transfer to complete if using DMA
    // If you're using DMA, uncomment this:
    // while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {}

    return 0;
}

/**
 * Get total number of blocks on SD card
 * @return Number of 512-byte blocks
 */
uint32_t sd_card_get_block_count(void) {
    HAL_SD_CardInfoTypeDef card_info;

    if (HAL_SD_GetCardInfo(&hsd, &card_info) == HAL_OK) {
        return card_info.LogBlockNbr;
    }

    return 0;
}

/**
 * Check if SD card is ready
 * @return true if ready, false otherwise
 */
bool sd_card_is_ready(void) {
#if ENABLE_SD_CARD
    return (HAL_SD_GetCardState(&hsd) == HAL_SD_CARD_TRANSFER);
#else
    return false;
#endif
}
