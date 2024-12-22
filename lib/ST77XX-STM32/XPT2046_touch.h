//
// Created by Angel Dust on 08/07/2024.
//

#ifndef TRX_FRONTEND_XPT2046_TOUCH_H
#define TRX_FRONTEND_XPT2046_TOUCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "hw/stm32.h"

#define XPT2046_TOUCH_CS_PORT TOUCH_CE_PORT
#define XPT2046_TOUCH_CS_PIN TOUCH_CE_PIN
#define XPT2046_TOUCH_IRQ_PORT TOUCH_IRQ_PORT
#define XPT2046_TOUCH_IRQ_PIN TOUCH_IRQ_PIN
#define XPT2046_TOUCH_DATA_IN_PORT TOUCH_DATA_IN_PORT
#define XPT2046_TOUCH_DATA_IN_PIN TOUCH_DATA_IN_PIN
#define XPT2046_TOUCH_DATA_OUT_PORT TOUCH_DATA_OUT_PORT
#define XPT2046_TOUCH_DATA_OUT_PIN TOUCH_DATA_OUT_PIN
#define XPT2046_TOUCH_CLK_PORT TOUCH_CLK_PORT
#define XPT2046_TOUCH_CLK_PIN TOUCH_CLK_PIN

// convert value at addr to little-endian (16-bit)
#define __LEu16(addr) (((((uint16_t)(*(((uint8_t *)(addr)) + 1)))) | (((uint16_t)(*(((uint8_t *)(addr)) + 0))) << 8U)))

// resulting integer width given as t, e.g. __FROUND(uint16_t, -1.3)
#define __FROUND(t, x) ((x) < 0.0F ? -((t)((-(x)) + 0.5F)) : (t)((x) + 0.5F))

typedef struct {
   union {
      uint16_t x;
      uint16_t width;
   };
   union {
      uint16_t y;
      uint16_t height;
   };
} xpt2046_two_dimension_t;

typedef enum {
   // orientation is based on position of board pins when looking at the screen
   isoNONE = -1,
   isoDown,
   isoPortrait = isoDown, // = 0
   isoRight,
   isoLandscape = isoRight, // = 1
   isoUp,
   isoPortraitFlip = isoUp, // = 2
   isoLeft,
   isoLandscapeFlip = isoLeft, // = 3
   isoCOUNT                    // = 4
} xpt2046_screen_orientation_t;

typedef enum { itpNONE = -1, itpNotPressed, itpPressed, itpCOUNT } xpt2046_touch_pressed_t;

typedef enum {
   itcNONE = -1,
   itcScalar,
   itc3Point,
   itcCOUNT,
} xpt2046_touch_calibration_t;

typedef struct {
   xpt2046_two_dimension_t min;
   xpt2046_two_dimension_t max;
} xpt2046_scalar_calibrator_t;

typedef struct {
   xpt2046_two_dimension_t scale;
   int32_t delta_x;
   int32_t delta_y;
   float alpha_x;
   float beta_x;
   float alpha_y;
   float beta_y;
} xpt2046_3point_calibrator_t;

typedef struct xpt2046 xpt2046_t;

typedef void (*xpt2046_touch_callback_t)(xpt2046_t *, uint16_t, uint16_t);

struct xpt2046 {
   SPI_HandleTypeDef *spi_hal;
   xpt2046_screen_orientation_t orientation;
   uint16_t width;
   uint16_t height;
   uint8_t averages = 3;
   xpt2046_two_dimension_t touch_coordinate;
   xpt2046_touch_calibration_t touch_calibration;
   xpt2046_scalar_calibrator_t touch_scalar;
   xpt2046_3point_calibrator_t touch_3point;
   xpt2046_touch_pressed_t touch_pressed;
   xpt2046_touch_callback_t touch_pressed_begin;
   xpt2046_touch_callback_t touch_pressed_end;
   bool power_on_between_reads = false;
};

typedef HAL_StatusTypeDef xpt2046_status_t;

xpt2046_t *xpt2046_touch_init(SPI_HandleTypeDef *spi_hal, xpt2046_screen_orientation_t orientation, uint8_t averages);

extern xpt2046_t xpt2046_touch;

void xpt2046_touch_check(xpt2046_t *lcd);

void xpt2046_set_touch_pressed_begin_callback(xpt2046_touch_callback_t callback);

void xpt2046_set_touch_pressed_end_callback(xpt2046_touch_callback_t callback);

xpt2046_touch_pressed_t xpt2046_touch_coordinate(xpt2046_t *lcd, uint16_t *x_pos, uint16_t *y_pos);

void xpt2046_calibrate_scalar(xpt2046_t *lcd, uint16_t min_x, uint16_t min_y, uint16_t max_x, uint16_t max_y);

void xpt2046_calibrate_3point(xpt2046_t *lcd, uint16_t scale_width, uint16_t scale_height, int32_t screen_a_x, int32_t screen_a_y, int32_t screen_b_x,
                              int32_t screen_b_y, int32_t screen_c_x, int32_t screen_c_y, int32_t touch_a_x, int32_t touch_a_y, int32_t touch_b_x,
                              int32_t touch_b_y, int32_t touch_c_x, int32_t touch_c_y);

#ifdef __cplusplus
}
#endif

#endif // TRX_FRONTEND_XPT2046_TOUCH_H
