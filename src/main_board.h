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
#include <sys/_stdint.h>

namespace main_board {
extern ShiftReg PowControlShiftReg;
extern Signal mode_signal;
extern Signal if_filter_signal;

void init();

void main_board_signal_static_callback(void *thisptr, void *args);

void set_modulation_mode(MODULATION_MODE mod_val, bool force);

MODULATION_MODE get_modulation_mode();

void set_squelch();

void set_power_ctrl(uint8_t, bool);

void set_filter();

void update();

void wakeup();

void sleep();

void toggle_dsp();

bool set_mode(MODE mode);

bool toggle_mode();

void set_mute(GPIO_PinState);

GPIO_PinState get_mute();

void enable_analog_mute(bool);

void set_frontend_path(radio::FRONTEND_PATH path);

bool change_frontend_gain(int direction);

radio::FRONTEND_PATH get_frontend_path();

void set_if_filter(radio::IF_FILTER filter);

bool setGPIOExpPin(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort, uint8_t pin, bool set);

bool setGPIOExpPin(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort, uint8_t pin, bool set, bool commit);

void commitGPIOExpPort(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort);

bool getGPIOExpPin(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort, uint8_t pin);

void setGPIOExpPort(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort, uint8_t value);

uint8_t getGPIOExpPort(MCP23017_HandleTypeDef *hmcp, uint8_t mcpPort, uint8_t pin);

int get_frontend_gain();
} // namespace main_board
#endif // TRX_FRONTEND_MAIN_BOARD_H
