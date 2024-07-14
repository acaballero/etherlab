// Copyright (c) 2018 Rudá Moura <ruda.moura@gmail.com>
// License: The MIT License (MIT)

#ifndef MCP32017_H
#define MCP32017_H

typedef struct MCP23017_HandleTypeDef MCP23017_HandleTypeDef; // Forward declaration needed as there are some cyclic dependency I have no time to solve

#include "../I2CBitBang/i2cbitbang.h"

// Registers
#define REGISTER_IODIRA        0x00
#define REGISTER_IODIRB        0x01
#define REGISTER_IPOLA        0x02
#define REGISTER_IPOLB        0x03
#define REGISTER_GPINTENA    0x04
#define REGISTER_GPINTENB    0x05
#define REGISTER_DEFVALA    0x06
#define REGISTER_DEFVALB    0x07
#define REGISTER_INTCONA    0x08
#define REGISTER_INTCONB    0x09
#define REGISTER_IOCONA        0x0A
#define REGISTER_IOCONB     0x0B
#define REGISTER_GPPUA        0x0C
#define REGISTER_GPPUB        0x0D
#define REGISTER_INTFA        0x0E
#define REGISTER_INTFB        0x0F
#define REGISTER_INTCAPA    0x10
#define REGISTER_INTCAPB    0x11
#define REGISTER_GPIOA        0x12
#define REGISTER_GPIOB        0x13
#define REGISTER_OLATA        0x14
#define REGISTER_OLATB        0x15
// Ports
#define MCP23017_PORTA            0x00
#define MCP23017_PORTB            0x01

// Address (A0-A2)
#define MCP23017_ADDRESS_20        0x20
#define MCP23017_ADDRESS_21        0x21
#define MCP23017_ADDRESS_22        0x22
#define MCP23017_ADDRESS_23        0x23
#define MCP23017_ADDRESS_24        0x24
#define MCP23017_ADDRESS_25        0x25
#define MCP23017_ADDRESS_26        0x26
#define MCP23017_ADDRESS_27        0x27

// I/O Direction
// Default state: MCP23017_IODIR_ALL_INPUT
#define  MCP23017_IODIR_ALL_OUTPUT    0x00
#define MCP23017_IODIR_ALL_INPUT    0xFF
#define MCP23017_IODIR_IO0_INPUT    0x01
#define MCP23017_IODIR_IO1_INPUT    0x02
#define MCP23017_IODIR_IO2_INPUT    0x04
#define MCP23017_IODIR_IO3_INPUT    0x08
#define MCP23017_IODIR_IO4_INPUT    0x10
#define MCP23017_IODIR_IO5_INPUT    0x20
#define MCP23017_IODIR_IO6_INPUT    0x40
#define MCP23017_IODIR_IO7_INPUT    0x80

// Input Polarity
// Default state: MCP23017_IPOL_ALL_NORMAL
#define MCP23017_IPOL_ALL_NORMAL    0x00
#define MCP23017_IPOL_ALL_INVERTED    0xFF
#define MCP23017_IPOL_IO0_INVERTED    0x01
#define MCP23017_IPOL_IO1_INVERTED    0x02
#define MCP23017_IPOL_IO2_INVERTED    0x04
#define MCP23017_IPOL_IO3_INVERTED    0x08
#define MCP23017_IPOL_IO4_INVERTED    0x10
#define MCP23017_IPOL_IO5_INVERTED    0x20
#define MCP23017_IPOL_IO6_INVERTED    0x40
#define MCP23017_IPOL_IO7_INVERTED    0x80

// Pull-Up Resistor
// Default state: MCP23017_GPPU_ALL_DISABLED
#define MCP23017_GPPU_ALL_DISABLED    0x00
#define MCP23017_GPPU_ALL_ENABLED    0xFF
#define MCP23017_GPPU_IO0_ENABLED    0x01
#define MCP23017_GPPU_IO1_ENABLED    0x02
#define MCP23017_GPPU_IO2_ENABLED    0x04
#define MCP23017_GPPU_IO3_ENABLED    0x08
#define MCP23017_GPPU_IO4_ENABLED    0x10
#define MCP23017_GPPU_IO5_ENABLED    0x20
#define MCP23017_GPPU_IO6_ENABLED    0x40
#define MCP23017_GPPU_IO7_ENABLED    0x80

typedef struct MCP23017_HandleTypeDef {
    i2cbitbang *i2cbb;
    uint8_t gpio[2];
    bool initialized[2] = {0, 0};
} MCP23017_HandleTypeDef;

uint32_t mcp23017_writereg(MCP23017_HandleTypeDef *hdev, uint8_t reg, uint8_t value);

uint32_t mcp23017_read(MCP23017_HandleTypeDef *hdev, uint16_t reg, uint8_t *data);

uint32_t mcp23017_write(MCP23017_HandleTypeDef *hdev, uint16_t reg, uint8_t *data);

uint32_t mcp23017_iodir(MCP23017_HandleTypeDef *hdev, uint8_t port, uint8_t iodir);

uint32_t mcp23017_ipol(MCP23017_HandleTypeDef *hdev, uint8_t port, uint8_t ipol);

uint32_t mcp23017_ggpu(MCP23017_HandleTypeDef *hdev, uint8_t port, uint8_t pu);

uint32_t mcp23017_read_gpio(MCP23017_HandleTypeDef *hdev, uint8_t port);

uint32_t mcp23017_write_gpio(MCP23017_HandleTypeDef *hdev, uint8_t port);

#endif // MCP32017_H