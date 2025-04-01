// Copyright (c) 2018 Rudá Moura <ruda.moura@gmail.com>
// License: The MIT License (MIT)

#include "mcp23017.h"

#define I2C_TIMEOUT 10

uint32_t mcp23017_read(MCP23017_HandleTypeDef *hdev, uint16_t reg, uint8_t *data) {

    hdev->i2cbb->readReg(reg, data);
    return hdev->i2cbb->getError();
}

uint32_t mcp23017_writereg(MCP23017_HandleTypeDef *hdev, uint8_t reg, uint8_t value) {
    hdev->i2cbb->writeReg(reg, value);
    return hdev->i2cbb->getError();
}

uint32_t mcp23017_iodir(MCP23017_HandleTypeDef *hdev, uint8_t port, uint8_t iodir) {
    hdev->i2cbb->writeReg(REGISTER_IODIRA | port, iodir);
    return hdev->i2cbb->getError();
}

uint32_t mcp23017_ipol(MCP23017_HandleTypeDef *hdev, uint8_t port, uint8_t ipol) {

    hdev->i2cbb->writeReg(REGISTER_IPOLA | port, ipol);
    return hdev->i2cbb->getError();
}

uint32_t mcp23017_ggpu(MCP23017_HandleTypeDef *hdev, uint8_t port, uint8_t pu) {

    hdev->i2cbb->writeReg(REGISTER_GPPUA | port, pu);
    return hdev->i2cbb->getError();
}

uint32_t mcp23017_read_gpio(MCP23017_HandleTypeDef *hdev, uint8_t port) {
    uint8_t data;

    hdev->i2cbb->readReg(REGISTER_GPIOA | port, &data);
    uint32_t status = hdev->i2cbb->getError();
    if (status == I2CBB_ERROR_NONE) {
        hdev->gpio[port] = data;
    }
    return status;
}

uint32_t mcp23017_write_gpio(MCP23017_HandleTypeDef *hdev, uint8_t port) {
    uint8_t data = hdev->gpio[port];
    hdev->i2cbb->writeReg(REGISTER_GPIOA | port, data);
    hdev->initialized[port] = true;
    return hdev->i2cbb->getError();
}
