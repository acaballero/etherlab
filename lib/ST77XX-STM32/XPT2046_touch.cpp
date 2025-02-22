// Created by Angel Dust on 08/07/2024.
//

#include "XPT2046_touch.h"
#include "ILI9341_fb.h"
#include "../printf/printf.h"

#define SWAP(a, b) (a += b, b = a - b, a -= b)

xpt2046_t xpt2046_touch;

void xpt2046_spi_touch_select(xpt2046_t *lcd);

void xpt2046_spi_touch_release(xpt2046_t *lcd);

uint16_t xpt2046_read_bitbang(uint8_t command);

uint16_t xpt2046_read_spi(uint8_t command);

static int32_t interp(int32_t x, int32_t x0, int32_t x1, int32_t y0, int32_t y1);

xpt2046_two_dimension_t xpt2046_clip_touch_coordinate(xpt2046_two_dimension_t coord, xpt2046_two_dimension_t min, xpt2046_two_dimension_t max);

xpt2046_two_dimension_t xpt2046_project_touch_coordinate(xpt2046_t *lcd, uint16_t x_pos, uint16_t y_pos);

xpt2046_t *xpt2046_touch_init(SPI_HandleTypeDef *spi_hal, xpt2046_screen_orientation_t orientation, uint8_t averages) {

    xpt2046_t *lcd = &xpt2046_touch;

    lcd->spi_hal = spi_hal;

    lcd->orientation = orientation;

    // Here we use original orientation sizes
    lcd->width = DISPLAY_Y_PIXELS;
    lcd->height = DISPLAY_X_PIXELS;

    lcd->averages = averages;
    lcd->touch_coordinate = (xpt2046_two_dimension_t){{0U}, {0U}};
    lcd->touch_calibration = itcScalar;
    lcd->touch_scalar = (xpt2046_scalar_calibrator_t){{{180}, {120}}, {{1870}, {1900}}};
    lcd->touch_3point = (xpt2046_3point_calibrator_t){{{0U}, {0U}}, 0, 0, 0.0F, 0.0F, 0.0F, 0.0F};

    lcd->touch_pressed = itpNotPressed;
    lcd->touch_pressed_begin = NULL;
    lcd->touch_pressed_end = NULL;

    lcd->power_on_between_reads = false;

    xpt2046_spi_touch_release(lcd);

    return lcd;
}

void xpt2046_touch_check(xpt2046_t *lcd) {
    uint16_t x_pos;
    uint16_t y_pos;

    // Read the new/incoming state of the touch screen
    xpt2046_touch_pressed_t pressed = xpt2046_touch_coordinate(lcd, &x_pos, &y_pos);

    // Switch path based on existing/prior state of the touch screen. Note this
    // requires the touch interrupt GPIO EXTI be set to detect both falling and
    // rising edges.
    switch (lcd->touch_pressed) {
        case itpNONE:
        case itpNotPressed:
            if (itpPressed == pressed) {
                // State change, start of press
                if (NULL != lcd->touch_pressed_begin) {
                    // use the current, normalized touch coordinate
                    lcd->touch_pressed_begin(lcd, x_pos, y_pos);
                }
            }
            break;

        case itpPressed:
            if ((itpNONE == pressed) || (itpNotPressed == pressed)) {
                // State change, end of press
                if (NULL != lcd->touch_pressed_end) {
                    // Use the last-known valid touch coordinate, since the current
                    // state does not have a valid touch.
                    lcd->touch_pressed_end(lcd, lcd->touch_coordinate.x, lcd->touch_coordinate.y);
                }
            }
            break;

        default:
            break;
    }

    // Update the internal state with current state of touch screen
    if (pressed != lcd->touch_pressed) {
        lcd->touch_pressed = pressed;
    }

    if (itpPressed == pressed) {
        lcd->touch_coordinate.x = x_pos;
        lcd->touch_coordinate.y = y_pos;
    }
}

xpt2046_touch_pressed_t xpt2046_touch_pressed(xpt2046_t *lcd) {
    if (NULL == lcd) {
        return itpNONE;
    }

    if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(XPT2046_TOUCH_IRQ_PORT, XPT2046_TOUCH_IRQ_PIN)) {
        return itpPressed;
    } else {
        return itpNotPressed;
    }
}

void xpt2046_set_touch_pressed_begin_callback(xpt2046_touch_callback_t callback) { xpt2046_touch.touch_pressed_begin = callback; }

void xpt2046_set_touch_pressed_end_callback(xpt2046_touch_callback_t callback) { xpt2046_touch.touch_pressed_end = callback; }

xpt2046_touch_pressed_t xpt2046_touch_coordinate(xpt2046_t *lcd, uint16_t *x_pos, uint16_t *y_pos) {
    static uint8_t x_cmd;
    static uint8_t y_cmd;

    if (NULL == lcd) {
        return itpNONE;
    }

    uint16_t req_samples = lcd->averages;
    if (lcd->averages == 0) {
        lcd->averages = 1;
    }

    if (lcd->power_on_between_reads) {
        x_cmd = 0xD3;
        y_cmd = 0x93;
    } else {
        x_cmd = 0xD0;
        y_cmd = 0x90;
    }

    static uint8_t sleep[] = {0xB0};

    uint32_t x_avg = 0U;
    uint32_t y_avg = 0U;

    uint16_t sample = req_samples;
    uint16_t num_samples = 0U;

    // Change SPI clock to 2MHz. The max rate supported by XPT2046 touch chipset
    // TODO: based on a 170MHz main tick
    if (lcd->spi_hal) {
        MODIFY_REG(lcd->spi_hal->Instance->CR1, SPI_CR1_BR, SPI_BAUDRATEPRESCALER_128);
    }

    HAL_GPIO_WritePin(XPT2046_TOUCH_CLK_PORT, XPT2046_TOUCH_CLK_PIN, GPIO_PIN_RESET);

    xpt2046_spi_touch_select(lcd);

    while ((itpPressed == xpt2046_touch_pressed(lcd)) && (sample--)) {

        uint16_t x;
        uint16_t y;

        if (lcd->spi_hal) {
            x = xpt2046_read_spi(x_cmd);
            y = xpt2046_read_spi(y_cmd);
        } else {
            x = xpt2046_read_bitbang(x_cmd);
            y = xpt2046_read_bitbang(y_cmd);
        }

        x_avg += x;
        y_avg += y;

        ++num_samples;
    }

    if (lcd->power_on_between_reads) {
        // We need to reset the IRQ line and put to sleep
        if (lcd->spi_hal) {
            HAL_SPI_Transmit(lcd->spi_hal, (uint8_t *)sleep, sizeof(sleep), HAL_MAX_DELAY);
        } else {
            for (int i = 7; i >= 0; i--) {
                HAL_GPIO_WritePin(XPT2046_TOUCH_DATA_IN_PORT, XPT2046_TOUCH_DATA_IN_PIN, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(XPT2046_TOUCH_CLK_PORT, XPT2046_TOUCH_CLK_PIN, GPIO_PIN_SET);
                delay_us(10);
                HAL_GPIO_WritePin(XPT2046_TOUCH_CLK_PORT, XPT2046_TOUCH_CLK_PIN, GPIO_PIN_RESET);
                delay_us(10);
            }
        }
    }

    xpt2046_spi_touch_release(lcd);

    if (lcd->spi_hal) {
        // Restore SPI clock prescaler
        MODIFY_REG(lcd->spi_hal->Instance->CR1, SPI_CR1_BR, lcd->spi_hal->Init.BaudRatePrescaler);
    }

    if (num_samples < req_samples) {
        return itpNotPressed;
    }

    x_avg = x_avg / req_samples;
    y_avg = y_avg / req_samples;

    xpt2046_two_dimension_t coord = xpt2046_project_touch_coordinate(lcd, x_avg, y_avg);

    *x_pos = coord.x;
    *y_pos = coord.y;

    printf_("touch: %d,%d %d,%d\n", x_avg, y_avg, *x_pos, *y_pos);

    return itpPressed;
}

void xpt2046_calibrate_scalar(xpt2046_t *lcd, uint16_t min_x, uint16_t min_y, uint16_t max_x, uint16_t max_y) {
    if (NULL == lcd) {
        return;
    }

    lcd->touch_calibration = itcScalar;
    lcd->touch_scalar.min = (xpt2046_two_dimension_t){{min_x}, {min_y}};
    lcd->touch_scalar.max = (xpt2046_two_dimension_t){{max_x}, {max_y}};
}

void xpt2046_calibrate_3point(xpt2046_t *lcd, uint16_t scale_width, uint16_t scale_height, int32_t screen_a_x, int32_t screen_a_y, int32_t screen_b_x,
                              int32_t screen_b_y, int32_t screen_c_x, int32_t screen_c_y, int32_t touch_a_x, int32_t touch_a_y, int32_t touch_b_x,
                              int32_t touch_b_y, int32_t touch_c_x, int32_t touch_c_y) {
    if (NULL == lcd) {
        return;
    }

    lcd->touch_calibration = itc3Point;

    lcd->touch_3point.scale = (xpt2046_two_dimension_t){{scale_width}, {scale_height}};

    int32_t delta = ((touch_a_x - touch_c_x) * (touch_b_y - touch_c_y)) - ((touch_b_x - touch_c_x) * (touch_a_y - touch_c_y));

    lcd->touch_3point.alpha_x = (float)(((screen_a_x - screen_c_x) * (touch_b_y - touch_c_y)) - ((screen_b_x - screen_c_x) * (touch_a_y - touch_c_y))) / delta;

    lcd->touch_3point.beta_x = (float)(((touch_a_x - touch_c_x) * (screen_b_x - screen_c_x)) - ((touch_b_x - touch_c_x) * (screen_a_x - screen_c_x))) / delta;

    lcd->touch_3point.delta_x = (float)((((int64_t)screen_a_x * ((touch_b_x * touch_c_y) - (touch_c_x * touch_b_y))) -
                                         ((int64_t)screen_b_x * ((touch_a_x * touch_c_y) - (touch_c_x * touch_a_y))) +
                                         ((int64_t)screen_c_x * ((touch_a_x * touch_b_y) - (touch_b_x * touch_a_y))))) /
                                    delta +
                                0.5;

    lcd->touch_3point.alpha_y = (float)(((screen_a_y - screen_c_y) * (touch_b_y - touch_c_y)) - ((screen_b_y - screen_c_y) * (touch_a_y - touch_c_y))) / delta;

    lcd->touch_3point.beta_y = (float)(((touch_a_x - touch_c_x) * (screen_b_y - screen_c_y)) - ((touch_b_x - touch_c_x) * (screen_a_y - screen_c_y))) / delta;

    lcd->touch_3point.delta_y = (float)((((int64_t)screen_a_y * (touch_b_x * touch_c_y - touch_c_x * touch_b_y)) -
                                         ((int64_t)screen_b_y * (touch_a_x * touch_c_y - touch_c_x * touch_a_y)) +
                                         ((int64_t)screen_c_y * (touch_a_x * touch_b_y - touch_b_x * touch_a_y)))) /
                                    delta +
                                0.5;
}

void xpt2046_spi_touch_select(xpt2046_t *lcd) { HAL_GPIO_WritePin(XPT2046_TOUCH_CS_PORT, XPT2046_TOUCH_CS_PIN, GPIO_PIN_RESET); }

void xpt2046_spi_touch_release(xpt2046_t *lcd) { HAL_GPIO_WritePin(XPT2046_TOUCH_CS_PORT, XPT2046_TOUCH_CS_PIN, GPIO_PIN_SET); }

static int32_t interp(int32_t x, int32_t x0, int32_t x1, int32_t y0, int32_t y1) {
    if (x1 == x0) {
        return 0;
    } // return 0 on divide-by-zero
    return (x - x0) * (y1 - y0) / (x1 - x0) + y0;
}

xpt2046_two_dimension_t xpt2046_clip_touch_coordinate(xpt2046_two_dimension_t coord, xpt2046_two_dimension_t min, xpt2046_two_dimension_t max) {
    if (coord.x < min.x) {
        coord.x = min.x;
    }
    if (coord.x > max.x) {
        coord.x = max.x;
    }
    if (coord.y < min.y) {
        coord.y = min.y;
    }
    if (coord.y > max.y) {
        coord.y = max.y;
    }

    return coord;
}

xpt2046_two_dimension_t xpt2046_project_touch_coordinate(xpt2046_t *lcd, uint16_t x_pos, uint16_t y_pos) {
    xpt2046_two_dimension_t coord = (xpt2046_two_dimension_t){{x_pos}, {y_pos}};
    //  xpt2046_two_dimension_t rotate;
    uint16_t x_scaled, y_scaled;

    if (NULL != lcd) {

        switch (lcd->touch_calibration) {
            case itcScalar:

                if (false) { // calibrate
                    if (coord.x < lcd->touch_scalar.min.x) {
                        lcd->touch_scalar.min.x = coord.x;
                    }
                    if (coord.x > lcd->touch_scalar.max.x) {
                        lcd->touch_scalar.max.x = coord.x;
                    }
                    // if (coord.y<lcd->touch_scalar.min.y) { // Min Y is OFF screen
                    //     lcd->touch_scalar.min.y=coord.y;
                    // }
                    if (coord.y > lcd->touch_scalar.max.y) {
                        lcd->touch_scalar.max.y = coord.y;
                    }
                } else {
                    coord = xpt2046_clip_touch_coordinate({{coord.x}, {coord.y}}, {{lcd->touch_scalar.min.x}, {lcd->touch_scalar.min.y}},
                                                          {{lcd->touch_scalar.max.x}, {lcd->touch_scalar.max.y}});
                }

                x_scaled = interp(coord.x, lcd->touch_scalar.min.x, lcd->touch_scalar.max.x, 0U, lcd->width);
                y_scaled = interp(coord.y, lcd->touch_scalar.min.y, lcd->touch_scalar.max.y, 0U, lcd->height);

                if (lcd->orientation == isoPortraitFlip || lcd->orientation == isoLandscapeFlip) {
                    x_scaled = lcd->width - x_scaled;
                } else {
                    y_scaled = lcd->height - y_scaled; // FIXME: This is just because i've swapped Y pins on the controller, but should go in the other branch
                }

                coord.x = x_scaled;
                coord.y = y_scaled;

                if (lcd->orientation == isoLandscapeFlip || lcd->orientation == isoLandscape) {
                    SWAP(coord.x, coord.y);
                }

                break;

            case itc3Point:

                //                coord = {
                //                        .x = (uint16_t) __FROUND(uint16_t, lcd->touch_3point.alpha_x * coord.x +
                //                                                           lcd->touch_3point.beta_x * coord.y +
                //                                                           lcd->touch_3point.delta_x
                //                        ),
                //                        .y = (uint16_t) __FROUND(uint16_t, lcd->touch_3point.alpha_y * coord.x +
                //                                                           lcd->touch_3point.beta_y * coord.y +
                //                                                           lcd->touch_3point.delta_y
                //                        )
                //                };
                //
                //                coord = xpt2046_clip_touch_coordinate(
                //                        rotate,
                //                        (xpt2046_two_dimension_t) {{0U},
                //                                                   {0U}},
                //                        lcd->touch_3point.scale);
                //
                //                switch (lcd->orientation % isoCOUNT) {
                //
                //                    case isoPortrait:
                //                        rotate = (xpt2046_two_dimension_t) {
                //                                .x = (uint16_t) (lcd->touch_3point.scale.width - coord.y),
                //                                .y = coord.x
                //                        };
                //                        break;
                //
                //                    case isoLandscape:
                //                        rotate = (xpt2046_two_dimension_t) {
                //                                .x = coord.x,
                //                                .y = coord.y
                //                        };
                //                        break;
                //
                //                    case isoPortraitFlip:
                //                        rotate = (xpt2046_two_dimension_t) {
                //                                .x = coord.y,
                //                                .y = (uint16_t) (lcd->touch_3point.scale.height - coord.x)
                //                        };
                //                        break;
                //
                //                    case isoLandscapeFlip:
                //                        rotate = (xpt2046_two_dimension_t) {
                //                                .x = (uint16_t) (lcd->touch_3point.scale.width - coord.x),
                //                                .y = (uint16_t) (lcd->touch_3point.scale.height - coord.y)
                //                        };
                //                        break;
                //                }

                break;

            default:
                break;
        }
    }

    return coord;
}

uint16_t xpt2046_read_spi(uint8_t command) {

    uint8_t result[2];
    HAL_SPI_Transmit(xpt2046_touch.spi_hal, &command, sizeof(&command), HAL_MAX_DELAY);
    HAL_SPI_Receive(xpt2046_touch.spi_hal, result, sizeof(result), HAL_MAX_DELAY);

    return *result;
}

uint16_t xpt2046_read_bitbang(uint8_t command) {
    uint16_t result = 0;

    for (int i = 7; i >= 0; i--) {
        HAL_GPIO_WritePin(XPT2046_TOUCH_DATA_IN_PORT, XPT2046_TOUCH_DATA_IN_PIN, static_cast<GPIO_PinState>(command & (1 << i)));
        HAL_GPIO_WritePin(XPT2046_TOUCH_CLK_PORT, XPT2046_TOUCH_CLK_PIN, GPIO_PIN_SET);
        delay_us(10);
        HAL_GPIO_WritePin(XPT2046_TOUCH_CLK_PORT, XPT2046_TOUCH_CLK_PIN, GPIO_PIN_RESET);
        delay_us(10);
    }

    delay_us(10);

    for (int i = 11; i >= 0; i--) {
        HAL_GPIO_WritePin(XPT2046_TOUCH_CLK_PORT, XPT2046_TOUCH_CLK_PIN, GPIO_PIN_SET);
        delay_us(10);
        result |= (HAL_GPIO_ReadPin(XPT2046_TOUCH_DATA_OUT_PORT, XPT2046_TOUCH_DATA_OUT_PIN) << i);
        HAL_GPIO_WritePin(XPT2046_TOUCH_CLK_PORT, XPT2046_TOUCH_CLK_PIN, GPIO_PIN_RESET);
        delay_us(10);
    }

    return result;
}
