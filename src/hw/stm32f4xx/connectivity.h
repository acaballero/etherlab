//
// Created by Angel Dust on 20/05/2021.
//

#ifndef TRX_FRONTEND_CONNECTIVITY_H
#define TRX_FRONTEND_CONNECTIVITY_H

#include <stm32f4xx.h>
#include "../lib/I2CBitBang/i2cbitbang.h"

extern i2cbitbang i2cport01;
extern i2cbitbang i2cport02;

extern SPI_HandleTypeDef hspi2;
extern SPI_HandleTypeDef hspi4;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

extern SD_HandleTypeDef hsd;

bool set_sdio_high_speed(bool);
void restart_sdio(bool high_speed);
void MS_SDIO_Init();
void MX_SPI2_Init();
void MX_SPI4_Init();
void MX_I2C1_Init();
void MX_I2C2_Init();
void BitBangI2C_setup();
bool setup_connectivity();

#ifdef __cplusplus
extern "C" {
#endif

void HAL_I2C_MspInit(I2C_HandleTypeDef *);
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *);
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi);
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi);
void SDIO_IRQHandler(void);
void SPI2_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif // TRX_FRONTEND_CONNECTIVITY_H
