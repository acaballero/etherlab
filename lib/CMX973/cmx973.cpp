#include "cmx973.h"

extern SPI_HandleTypeDef CMX973_SPI_HANDLE; // Defined in the parent project

st_cms973Params cmx973State;

/***************************************************************************//**
 * @brief assert SPI CS
 * This set the IO pin to zero..
*******************************************************************************/
void cmx973_SPI_assert_CS() {
	HAL_GPIO_WritePin(CMX973_CS_GPIO_PORT, CMX973_CS_PIN, GPIO_PIN_RESET);
}

/***************************************************************************//**
 * @brief assert SPI CS
 * This sets the IO pin to one..
*******************************************************************************/
void cmx973_SPI_deassert_CS() {
	HAL_GPIO_WritePin(CMX973_CS_GPIO_PORT, CMX973_CS_PIN, GPIO_PIN_SET);
}

/************************
 *  @brief write 8 bits of data on SPI interface MOSI pin..
 */
HAL_StatusTypeDef cmx973_SPI_Write(uint8_t data) {
    HAL_StatusTypeDef ret = HAL_SPI_Transmit(&CMX973_SPI_HANDLE, &data, 1, 1000);
    return ret;
}



/***************************************************************************//**
 * @brief Sends a command
 *
 * @param cmd - Data value to write.
 *
 * @return Returns 0 in case of success or error code.
*******************************************************************************/
uint8_t cmx973_write_cmd(uint8_t cmd)
{
    cmx973_SPI_assert_CS();
    cmx973_SPI_Write(cmd);
    cmx973_SPI_deassert_CS();
    return 0;
}

/***************************************************************************//**
 * @brief Writes a register of the CMX973.
 *
 * @param reg - Register address to write.
 *
 * @param data - Data value to write.
 *
 * @return Returns 0 in case of success or error code.
*******************************************************************************/
uint8_t cmx973_write_reg(uint8_t reg,uint8_t data)
{
    // Assert CS and LD for whole period..

    HAL_StatusTypeDef ret;

    cmx973_SPI_assert_CS();
    ret = cmx973_SPI_Write(reg);
    if (ret==HAL_OK) {
        ret = cmx973_SPI_Write(data);
    }
    cmx973_SPI_deassert_CS();

    return ret;
}


/***************************************************************************//**
 * @brief Reads a register of the CMX973.
 *
 * @param reg - Register address to read.
 *
 * @param data - Data value to read.
 *
 * @return Returns 0 in case of success or error code.
*******************************************************************************/
uint8_t cmx973_read_reg(uint8_t reg,uint8_t &data)
{
    // Assert CS and LD for whole period..

    cmx973_SPI_assert_CS();
    HAL_SPI_Transmit(&CMX973_SPI_HANDLE, &reg, 1, 1000);
    HAL_SPI_Receive(&CMX973_SPI_HANDLE, &data, 1, 1000);
    cmx973_SPI_deassert_CS();
    return 0;
}

uint8_t cmx973_update() {
    uint8_t ret=0;
    ret = cmx973_write_reg(CMX973_GCR,cmx973State.gcr);
    if (!ret) ret = cmx973_write_reg(CMX973_RXC,cmx973State.rxc);

    return ret;
}

void cmx973_sleep() {
    cmx973_write_cmd(0x1A);
}

void cmx973_wakeup() {
    cmx973_update();
}


