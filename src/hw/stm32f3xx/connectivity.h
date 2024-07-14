//
// Created by Angel Dust on 20/05/2021.
//

#ifndef TRX_FRONTEND_CONNECTIVITY_H
#define TRX_FRONTEND_CONNECTIVITY_H


#ifdef __cplusplus
extern "C" {
#endif

void HAL_I2C_MspInit(I2C_HandleTypeDef *);
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *);
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi);
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi);
void HAL_SD_MspInit(SD_HandleTypeDef* hsd);
void HAL_SD_MspDeInit(SD_HandleTypeDef* hsd);


#ifdef __cplusplus
}
#endif


extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;

void MX_SPI1_Init();
void MX_I2C1_Init();
void MX_SDIO_SD_Init();
void BitBangI2C_setup();

void setup_connectivity();



#endif //TRX_FRONTEND_CONNECTIVITY_H
