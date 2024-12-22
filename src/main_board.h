//
// Created by Angel Dust on 23/06/2021.
//

#ifndef TRX_FRONTEND_MAIN_BOARD_H
#define TRX_FRONTEND_MAIN_BOARD_H

#include "radio.h"
#include "types.h"
#include "stdio.h"
#include "../lib/MCP23017/mcp23017.h"
#include "ShiftReg.h"

namespace main_board {
extern ShiftReg PowControlShiftReg;

void init();

void main_board_signal_static_callback(void *thisptr, void *args);

void setModulationMode(int mod_val, bool force);

MODULATION_MODE getModulationMode();

void setSquelch();

void setPowerCtrl(uint8_t, bool);

void set_filter();

void update();

void wakeup();

void sleep();

bool setMode(MODE mode);

void setMute(GPIO_PinState);

GPIO_PinState getMute();

void set_if_filter(radio::IF_FILTER filter);

bool setGPIOExpPin(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort, uint8_t pin,
                   bool set);

bool setGPIOExpPin(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort, uint8_t pin,
                   bool set, bool commit);

void commitGPIOExpPort(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort);

bool getGPIOExpPin(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort, uint8_t pin);

void setGPIOExpPort(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort,
                    uint8_t value);

uint8_t getGPIOExpPort(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort,
                       uint8_t pin);
} // namespace main_board
#endif // TRX_FRONTEND_MAIN_BOARD_H
