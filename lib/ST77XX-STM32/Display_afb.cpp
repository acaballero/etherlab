#include "Display_afb.h"
#include <string.h>
#include <math.h>
#include <hw/stm32.h>
#include <stdint.h>
#include <algorithm>
#include "ILI9341_fb.h"
#include "Painter.hpp"
#include "stm32f4xx_hal_def.h"
#include "../utils/utils.hpp"

#define min2(a, b) ((a) < (b) ? (a) : (b))
#define max2(a, b) ((a) > (b) ? (a) : (b))

#define SETPIXEL(x, y, c) (*(this->curr_buffer + x + (y >> 16)) = c)

uint16_t palette16[16] = {C565_WHITE, C565_RED,  C565_GOLD,       C565_GREY_DARKER, C565_BLUE, C565_PURPLE, C565_GREY_DARK, C565_GREY_LIGHT,
                          C565_PINK,  C565_NAVY, C565_GREEN_DARK, C565_CYAN_DARK,   C565_BLUE, C565_GREEN,  C565_CYAN,      C565_RED};

Display::Display(SPI_HandleTypeDef *spi_port) {
    this->spi_port = spi_port;
    this->curr_buffer = b565_buffer;
}

void Display::convertPalette888to565(const uint32_t *orig, uint16_t *dest, uint8_t size) {
    for (int i = 0; i < size; i++) {
        dest[i] = SWAP_BYTES(RGB888_TO_RGB565(orig[i]));
    }
}

void Display::set_transparency(uint8_t v) {
    transparency = v;
}

uint8_t Display::get_transparency() {
    return transparency;
}

void Display::clear(Color color) {

    if (ow == 0) {
        // We are drawing in the whole area so we can just memset

        memset(this->curr_buffer, color, this->chunk_height * this->curr_area->box.width * 2);
    } else {
        // The memory of the rectangle is not contiguous in the area
        fillBuffer(color);
    }
}

void Display::setOffset(Box r) {

    ox = r.x;
    oy = r.y;
    ow = r.width;
    oh = r.height;
}

bool Display::hasOffset() {
    return ow;
}

Box Display::get_offset() {
    return {ox, oy, ow, oh};
}

void Display::clear_offset() {
    ox = 0;
    oy = 0;
    ow = 0;
    oh = 0;
}

void Display::set_enabled(bool b) {
    this->enabled = b;
}

bool Display::getEnabled() {
    return this->enabled;
}

bool Display::draw_area(Area *area, Painter *painter) {
    return draw_area(area, painter, true);
}

uint32_t Display::calculate_buffer_checksum() {
    // Enable CRC peripheral clock if not already enabled
    __HAL_RCC_CRC_CLK_ENABLE();

    // Reset CRC calculation
    CRC->CR = CRC_CR_RESET;

    // Calculate CRC32 of current buffer
    uint32_t *buffer32 = (uint32_t *)curr_buffer;
    uint32_t words = (chunk_height * curr_area->box.width + 1) / 2; // Convert 16-bit pixels to 32-bit words

    for (uint32_t i = 0; i < words; i++) {
        CRC->DR = buffer32[i];
    }

    return CRC->DR; // Read final CRC32 result
}

void Display::check_dma_transfer_length() {
    // The last chunk may need fewer bytes to transfer
    if (curr_area->box.height - current_line < chunk_height << 1) {
        uint16_t dma_buffer_size = (curr_area->box.height - current_line) * curr_area->box.width;
        dma_transfer_length = dma_buffer_size << 1;
    }
}

void Display::spi_transfer(uint16_t size) {
    // Transfer the buffer without DMA
    // Experimental: Just to see if I manage to share one SPI bus with two devices, one of which transfers within an interrupt
    for (uint32_t i = 0; i < size; i++) {

        // HAL_TIM_Base_Stop_IT(&htim15);
        DISP_CE_PORT->BSRR |= DISP_CE_PIN << 16;

        if ((spi_port->Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE) {
            spi_port->Instance->CR1 |= SPI_CR1_SPE; // enable SPI
        }
        *(__IO uint8_t *)&spi_port->Instance->DR = *((__IO uint8_t *)curr_buffer + i); // Write data to be transmitted to the SPI data register
        // while (!(spi_port->Instance->SR & (SPI_SR_TXE)));     // Wait until transmit complete
        // while (!(spi_port->Instance->SR & (SPI_SR_RXNE)));    // Wait until receive complete
        while (spi_port->Instance->SR & (SPI_SR_BSY)) {
            ; // Wait until SPI is not busy anymore
        }
        uint8_t rxDat = *(__IO uint8_t *)&spi_port->Instance->DR; // Return received data from SPI data register
        UNUSED(rxDat);
        DISP_CE_PORT->BSRR |= DISP_CE_PIN;
        //  HAL_TIM_Base_Start_IT(&htim15);
    }
}

bool Display::draw_area(Area *area, Painter *painter, bool pad_display) {

    if (this->enabled) {

        this->curr_area = area;

        this->gotoXY(0, 0);

        uint16_t x = area->box.x;
        uint16_t y = area->box.y;

        if (pad_display) {
            x += DISPLAY_PADDING;
            y += DISPLAY_PADDING;
        }

        // Number of pixels in the area
        uint32_t buffer_size_pixels_remaining = area->size;

        // Number of bytes of the block sent to the driver each transfer. This is limited by the memory available

        // We set the buffer size to be a whole number of lines of the area so we can easily determine whether we can write
        // or not, depending on the relative position of the buffer in the area

        // chunk_height = integer floor of buffer_size/2/width (half buffer lines)
        uint32_t w = area->box.width;

        uint32_t a = b565_buffer_size / 2;
        uint32_t d = a / w;
        this->chunk_height = d * w == a ? d : d - ((a < 0) ^ (w < 0));

        // chunk_height should not be greater than the area height.
        // An area can be small enough (less than half the buffer size) that it can be drawn in a single DMA transfer
        if (this->chunk_height > area->box.height) {
            this->chunk_height = area->box.height;
        }

        uint32_t max_buffer_size = this->chunk_height * w;

        uint32_t dma_buffer_size = buffer_size_pixels_remaining < max_buffer_size ? buffer_size_pixels_remaining : max_buffer_size;

        uint32_t half_dma_buffer_size = max_buffer_size;

        dma_buffer_size *= 2; // we will draw two chunks per DMA transfer

        dma_buffer_size = min2(dma_buffer_size, area->size); // But we won't transfer more than the area size

        dma_transfer_length = dma_buffer_size * 2; // 2 bytes per pixel

        if (setAddressWindow(x, y, x + area->box.width - 1, y + area->box.height - 1) != HAL_OK) {
            return false;
        }

        if (use_dma) {
            InitDisplayDataTransfer();
        } else {
            DISP_DC_PORT->BSRR |= DISP_DC_PIN; // DC PIN SET
        }

        current_line = 0;
        // uint16_t dy = area->y;

        curr_buffer = b565_buffer;

#if DEBUG_LCD
        char str[10];
        if (area->show_fps) {
            sprintf(str, "%2d.%d", (int)area->fps, (int)(area->fps * 10) % 10);
        }
#endif

        bool address_window_set = true;

        while (current_line < area->box.height) {

            current_last_line = min2(current_line + chunk_height, area->box.height) - 1;

            // Prevent any interruption of the paint callback

            // interrupted = false;
            //  NVIC_DisableIRQ(TIM8_TRG_COM_TIM14_IRQn);
            busy = true; // Not fully atomic. Disable interrupt for proper atomic behavior

            painter->paint_callback();

            // IMPORTANT: The callback must be executed faster than the half transfer of the DMA buffer. Otherwise, the buffer
            // won't be fully written when the DMA transfers it.
            // (This can be a problem if the compilation is set without speed optimizations and, be aware, these
            // optimizations need sometimes to be turned off to be able to properly debug with breakpoints)

#if DEBUG_LCD
            if (area->show_fps) {
                writeString(0, curr_area->box.height - 11 - oy, str, (FontDef *)&Font_7x10, C565_BLACK, C565_WHITE);
                writeLine(0, curr_area->box.height - 12 - oy, 24, curr_area->box.height - 12, C565_WHITE);
            }
#endif
            busy = false;
            // NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);

            // if (interrupted) {
            //     continue;
            // }
#if 0
            if (enable_skips) {

                if (curr_buffer == b565_buffer) {
                    int16_t slice_index = find_zone(area->box.x, area->box.y, current_line);
                    uint32_t current_checksum = calculate_buffer_checksum();

                    if (slice_index >= 0 && slice_checksums[slice_index].checksum == current_checksum) {
                        // Zone unchanged - skip DMA transfer

                        // NOTE: This optimization does not have much impact on the performance. The current buffer still needs to be painted to calculate the
                        // checksum and that, except for the first half slice, is done in parallel with the DMA transfer. Also, when a slice is skipped, we need
                        // to wait for a current transfer to stop and then start a new DMA transfer. All in all, the performance gain is probably not worth the
                        // added Also, we are only skipping the first half buffers (curr_buffer == b565_buffer) complexity. This "slice skipping" approach has
                        // been implemented for widgets that, albeit dirty, only update a small portion of their area (e.g. plots, fft...)

                        address_window_set = false;

                        current_line += chunk_height;

                        check_dma_transfer_length();

                        continue;
                    }

                    // Store new checksum for this zone
                    if (slice_index >= 0) {
                        slice_checksums[slice_index].checksum = current_checksum;
                    } else {
                        slice_checksums[next_slice_index] = {area->box.x, area->box.y, current_line, current_checksum};
                        next_slice_index = (next_slice_index + 1) % MAX_SLICES;
                    }
                }

                // Set address window for this zone
                if (!address_window_set) {
                    // First transfer - set window for remaining area
                    END_DMA_TRANSFER
                    //  printf_("Last zone was skipped: Setting window(%d,%d,%d,%d)\n", x, y + current_line, x + area->box.width - 1, y + area->box.height - 1);
                    setAddressWindow(x, y + current_line, x + area->box.width - 1, y + area->box.height - 1);

                    InitDisplayDataTransfer();
                    address_window_set = true;
                }
            }
#endif
            current_line += chunk_height;

            if (!use_dma) {
                spi_transfer(dma_buffer_size);
            }

            if (curr_buffer == b565_buffer) {

                // The first half of the RGB buffer is ready to be transferred
                // If DMA is ready (second half has been transferred, so state == READY), we start another transfer now

                curr_buffer = b565_buffer + half_dma_buffer_size;

                if (use_dma) {

                    while (HAL_SPI_GetState(spi_port) != HAL_SPI_STATE_READY) {
                        // Wait for previous DMA finished
                    }

                    DMAHalfTransferCompleted = false;

                    HAL_SPI_Transmit_DMA(spi_port, ((uint8_t *)b565_buffer), dma_transfer_length);
                }
            } else {

                // The second half of the RGB buffer is ready
                // Wait for the first half of the buffer to be transferred before start again

                curr_buffer = b565_buffer;

                if (use_dma) {
                    while (!DMAHalfTransferCompleted) {
                    }
                }

                // The last chunk may need fewer bytes to transfer
                check_dma_transfer_length();
            }
        }

        if (use_dma) {
            END_DMA_TRANSFER
        } else {
            DISP_DC_PORT->BSRR |= DISP_DC_PIN << 16; // DC PIN UNSET
        }
    }

    // DMA is the default write mode. When disabled, it is enabled again after every redraw and must be set again before the next
    use_dma = true;

    return true;
}

// Function to draw a single corner using midpoint circle algorithm
void Display::draw_corner(int16_t centerX, int16_t centerY, uint8_t radius, uint8_t quadrant, bool filled) {

    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (y >= x) {
        if (filled) {
            switch (quadrant) {
                case 1: // Top-right
                    writeLine(centerX, centerY - y, centerX + x, centerY - y);
                    writeLine(centerX, centerY - x, centerX + y, centerY - x);
                    break;
                case 2: // Top-left
                    writeLine(centerX - x, centerY - y, centerX, centerY - y);
                    writeLine(centerX - y, centerY - x, centerX, centerY - x);
                    break;
                case 3: // Bottom-left
                    writeLine(centerX - x, centerY + y, centerX, centerY + y);
                    writeLine(centerX - y, centerY + x, centerX, centerY + x);
                    break;
                case 4: // Bottom-right
                    writeLine(centerX, centerY + y, centerX + x, centerY + y);
                    writeLine(centerX, centerY + x, centerX + y, centerY + x);
                    break;
            }
        } else {
            switch (quadrant) {
                case 1: // Top-right
                    setPixel(centerX + x, centerY - y, color);
                    setPixel(centerX + y, centerY - x, color);
                    break;
                case 2: // Top-left
                    setPixel(centerX - x, centerY - y, color);
                    setPixel(centerX - y, centerY - x, color);
                    break;
                case 3: // Bottom-left
                    setPixel(centerX - x, centerY + y, color);
                    setPixel(centerX - y, centerY + x, color);
                    break;
                case 4: // Bottom-right
                    setPixel(centerX + x, centerY + y, color);
                    setPixel(centerX + y, centerY + x, color);
                    break;
            }
        }

        if (d < 0) {
            d += 4 * x + 6;
        } else {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void Display::drawRoundedRectangle(int16_t x0, int16_t y0, uint16_t width, uint16_t height, uint16_t radius, bool filled) {
    drawRoundedRectangle(x0, y0, width, height, radius, filled, true, true, true, true);
}

void Display::drawRoundedRectangle(int16_t x0, int16_t y0, uint16_t width, uint16_t height, uint16_t radius, bool filled, bool top_left, bool top_right,
                                   bool bottom_left, bool bottom_right) {

    int x1 = x0 + radius, y1 = y0 + radius;
    int x2 = x0 + width - radius, y2 = y0 + height - radius;

    // Draw the rectangular body without corners
    if (filled) {
        for (int y = 0; y < height; y++) {
            int startX = ((top_left && y < radius) || (bottom_left && y >= height - radius)) ? radius : 0;
            int endX = ((top_right && y < radius) || (bottom_right && y >= height - radius)) ? width - radius : width;
            writeLine(x0 + startX, y0 + y, x0 + endX - 1, y0 + y);
        }
    } else {
        // Draw the straight lines for the sides and top/bottom
        writeLine(x1, y0, x2, y0);                           // Top
        writeLine(x1, y0 + height - 1, x2, y0 + height - 1); // Bottom
        writeLine(x0, y1, x0, y2);                           // Left
        writeLine(x0 + width - 1, y1, x0 + width - 1, y2);   // Right
    }
    // Draw the four cornerscd
    if (top_right) {
        draw_corner(x0 + width - radius - 1, y0 + radius, radius, 1, filled); // Top-right
    }
    if (top_left) {
        draw_corner(x0 + radius, y0 + radius, radius, 2, filled); // Top-left
    }
    if (bottom_left) {
        draw_corner(x0 + radius, y0 + height - radius - 1, radius, 3, filled); // Bottom-left
    }
    if (bottom_right) {
        draw_corner(x0 + width - radius - 1, y0 + height - radius - 1, radius, 4, filled); // Bottom-right
    }
}

void Display::DMATxHalfCpltCallback(void) {
    this->DMAHalfTransferCompleted = true;
    // GPIOB->BSRR= GPIO_PIN_5 << 16;
}

void Display::DMATxCpltCallback(void) {
}

uint16_t Display::getColor() {
    return this->color;
}

void Display::fillBuffer(Color c) {

    if (ow == 0) {
        // No offset, so fill the whole buffer
        for (uint16_t i = 0; i < this->chunk_height * this->curr_area->box.width; i++) {
            *(curr_buffer + i) = c;
        }
    } else {
        // Partial buffer fill
        fill(ox > 0 ? 0 : -ox, oy > 0 ? 0 : -oy, (ox > 0 ? 0 : -ox) + ow - 1, (oy > 0 ? 0 : -oy) + oh - 1, c);
    }
}

void Display::fill(DisplayPoint p, DisplaySize s, Color c) {
    fill(p.x, p.y, p.x + s.w - 1, p.y + s.h - 1, c);
}
void Display::fill(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color c) {

    x1 += ox;
    y1 += oy;
    x2 += ox;
    y2 += oy;

    uint16_t zy1 = this->current_line;
    uint16_t zy2 = this->current_last_line;

    if (y2 >= zy1 && y1 <= zy2) { // There's something to draw in the current chunk

        y1 = max2(y1, this->current_line);
        uint16_t dy1 = y1 - this->current_line;
        uint16_t dy2 = y2 - this->current_line;
        uint16_t *p = this->curr_buffer + (dy1 * this->curr_area->box.width + x1);
        size_t row_bytes = (x2 - x1 + 1) << 1;
        uint16_t *p2 = this->curr_buffer + min2(this->chunk_height * this->curr_area->box.width, dy2 * this->curr_area->box.width + x2);

        while (p < p2) {

            if (c >> 8 == c && 0x0F) { // repeated-byte colors can be written by memset
                memset(p, c, row_bytes);
            } else {
                std::fill(p, p + (row_bytes >> 1), c);
            }
            p += this->curr_area->box.width;
        }
    }
}

void Display::setPixel(int16_t x, int16_t y, uint16_t c) {

    // Have in mind this function is too slow (around 17 assembler instructions) to call
    // it within a area drawing callback function (in which you have around 18 instructions to draw a pixel ( 72Mhz(core) / ( 18Mhz(spi) * 16
    // (bits/pixel) ) / 4 (cycles per instruction, but can be slower due to bus waiting) ) So, if we have to fill a area, use the buffer
    // (getBuffer) instead with incremental offset
    x += ox;
    y += oy;

    uint16_t width = this->curr_area->box.width;

    if (y >= this->current_line && y <= this->current_last_line) {

        uint16_t dy = y - this->current_line;

        // index of the pixel in the DMA buffer
        uint16_t ix = ((dy * width) + x); // >> buffer->log2_pixels_per_byte;

        *(curr_buffer + ix) = c;
    }
}

void Display::writeLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    writeLine(x1, y1, x2, y2, this->color);
}

void Display::writeVertLine(int16_t x, int16_t y1, int16_t y2, uint16_t color) {

    x += ox;
    y1 += oy;
    y2 += oy;

    int y = y1;
    if (y1 > y2) {
        y2 = y1;
        y = y2;
    }

    Area *area = this->curr_area;

    uint16_t delta = area->box.width;

    if (y2 >= this->current_line && y <= this->current_last_line) {

        y2 = min2(this->current_last_line, y2);
        y = max2(this->current_line, y);

        uint16_t dy = y - this->current_line;
        uint16_t ix = dy * delta + x;

        while (y <= y2) {

            *(curr_buffer + ix) = color;

            y++;
            ix += delta;
        }
    }
}

void Display::writeLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    this->writeLine(x1, y1, x2, y2, color, 1);
}

void Display::writeLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color, uint8_t) {

    x1 += ox;
    y1 += oy;
    x2 += ox;
    y2 += oy;

    uint16_t rect_width = this->curr_area->box.width;

    int zy1 = this->current_line;
    int zy2 = this->current_last_line;

    if (y1 > y2) {
        uint16_t aux = y1;
        y1 = y2;
        y2 = aux;
        aux = x1;
        x1 = x2;
        x2 = aux;
    }

    if (y2 >= zy1 && y1 <= zy2) { // There's something to draw in the current chunk

        /* Fastest line drawing algorithm (supporting all slopes) I've encountered so far (http://www.edepot.com/linee.html) */

        bool yLonger = false;
        int shortLen = y2 - y1;
        int longLen = x2 - x1;
        if (abs(shortLen) > abs(longLen)) {
            int swap = shortLen;
            shortLen = longLen;
            longLen = swap;
            yLonger = true;
        }
        int decInc;
        if (longLen == 0) {
            decInc = 0;
        } else {
            decInc = (shortLen << 16) / longLen;
        }

        int16_t ipx = x1;
        int16_t ipy = y1;

        if (yLonger) {
            if (longLen > 0) {
                longLen += ipy;
                for (int j = 0x8000 + (x1 << 16); ipy <= longLen; ++ipy) {
                    if (ipy >= zy1 && ipy <= zy2) {
                        ipx = (ipy - this->current_line) * rect_width; // offset in the buffer
                        *(this->curr_buffer + ipx + (j >> 16)) = color;
                    }

                    j += decInc;
                }
                return;
            }
            longLen += ipy;
            for (int j = 0x8000 + (x1 << 16); ipy >= longLen; --ipy) {
                if (ipy >= zy1 && ipy <= zy2) {
                    ipx = (ipy - this->current_line) * rect_width; // offset in the buffer
                    *(this->curr_buffer + ipx + (j >> 16)) = color;
                }

                j -= decInc;
            }
            return;
        }

        // xLonger
        if (longLen > 0) {
            for (int j = 0x8000 + (y1 << 16); x1 <= x2; ++x1) {
                ipy = (j >> 16);
                if (ipy >= zy1 && ipy <= zy2) {
                    ipx = (ipy - this->current_line) * rect_width; // offset in the buffer
                    *(this->curr_buffer + ipx + x1) = color;
                }
                j += decInc;
            }
            return;
        }

        for (int j = 0x8000 + (y1 << 16); x2 >= x1; --x2) {
            ipy = (j >> 16);
            if (ipy >= zy1 && ipy <= zy2) {
                ipx = (ipy - this->current_line) * rect_width; // offset in the buffer
                *(this->curr_buffer + ipx + x2) = color;
            }
            j -= decInc;
        }

        /* Simpler, but slower line drawing algorithm */
        //        uint16_t t;
        //        int16_t xerr = 0, yerr = 0, delta_x, delta_y, distance;
        //        int8_t incx, incy;
        //        uint16_t uRow, uCol, y;
        //
        //        delta_y = y2 - y1;
        //        delta_x = x2 - x1;
        //
        //        uRow = x1;
        //        uCol = y1 - area->current_line;
        //        y = y1;
        //
        //        if (delta_x > 0)
        //            incx = 1;
        //        else if (delta_x == 0)
        //            incx = 0;
        //        else {
        //            incx = -1;
        //            delta_x = -delta_x;
        //        }
        //
        //
        //        if (delta_y > 0)
        //            incy = 1;
        //        else if (delta_y == 0)
        //            incy = 0;
        //        else {
        //            incy = -1;
        //            delta_y = -delta_x;
        //        }
        //
        //        if (delta_x > delta_y)
        //            distance = delta_x;
        //        else
        //            distance = delta_y;
        //
        //        uint16_t ay = uCol * area->width;
        //        for (t = 0; t <= distance; t++) {
        //
        //            if (y >= zy1 && y <= zy2) {
        //
        //                uint16_t ix = (ay + uRow); // >> buffer->log2_pixels_per_byte;
        //
        //                *(this->curr_buffer + ix) = color;
        //            }
        //
        //            xerr += delta_x;
        //            yerr += delta_y;
        //            if (xerr > distance) {
        //                xerr -= distance;
        //                uRow += incx;
        //            }
        //            if (yerr > distance) {
        //                yerr -= distance;
        //                uCol += incy;
        //                ay +=  area->width;
        //                y += incy;
        //            }
        //
        //
        //        }
    }
}

void Display::writeRect(DisplayPoint p, DisplaySize s, Color c) {
    // select();
    writeRect(p.x, p.y, p.x + s.w, p.y + s.h, c);
}

void Display::writeRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    // select();
    writeRect(x1, y1, x2, y2, C565_WHITE);
}

void Display::writeRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    // select();
    writeLine(x1, y1, x2, y1, color);
    writeLine(x1, y1, x1, y2, color);
    writeLine(x1, y2, x2, y2, color);
    writeLine(x2, y1, x2, y2, color);
    // Unselect();
}

void Display::drawCircle(int16_t x0, int16_t y0, uint8_t r, uint16_t color) {
    // select();
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    setPixel(x0, y0 + r, color);
    setPixel(x0, y0 - r, color);
    setPixel(x0 + r, y0, color);
    setPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        setPixel(x0 + x, y0 + y, color);
        setPixel(x0 - x, y0 + y, color);
        setPixel(x0 + x, y0 - y, color);
        setPixel(x0 - x, y0 - y, color);

        setPixel(x0 + y, y0 + x, color);
        setPixel(x0 - y, y0 + x, color);
        setPixel(x0 + y, y0 - x, color);
        setPixel(x0 - y, y0 - x, color);
    }
    // Unselect();
}

void Display::invertColors(uint8_t invert) {
    // select();
    writeCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
    // Unselect();
}

void Display::writeChar(char ch) {

    this->writeChar(px, py, ch, font, color, bgColor);
    this->px += font->width;
}

void Display::writeChar(int16_t x, int16_t y, char ch, const FontDef *font, uint16_t color, uint16_t bgcolor) {

    x += ox;
    y += oy;

    uint16_t i, b, j;
    int px, py = y, dy, y1, y2;
    uint16_t c;

    uint16_t width = this->curr_area->box.width;

    y1 = this->current_line;
    y2 = this->current_line + this->chunk_height;
    uint8_t start_char = font->start_char;

    if (py + font->height >= y1 && py < y2) {

        int si = max2(0, y1 - py);
        int ei = min2(font->height, y2 - py);
        int sdy = dy = max2(0, py - this->current_line);
        uint16_t mask;
        const uint8_t *data = ((FontDef8 *)font)->data;

        uint8_t incr = font->size - 1;

        if (font->encoding == ROWS) {

            mask = font->size == 2 ? 0x8000 : 0x80;
            data = data + (((ch - start_char) * font->height) << incr);

            for (i = si; i < ei; i++, dy++) {

                b = font->size == 2 ? ((uint16_t *)data)[i] : ((uint8_t *)data)[i << incr];

                // Commas and periods are too width in monospaced fonts. So we remove
                // columns from each side
                uint8_t j1, jn;
                if (trim_enabled && (ch == ',' || ch == '.' || ch == ':' || ch == ' ')) {
                    j1 = font->trim_punct_start;
                    jn = font->width - font->trim_punct_end;
                } else {
                    j1 = 0;
                    jn = font->width;
                }

                for (j = j1, px = x; j < jn; j++, px++) {

                    if ((b << j) & mask) {
                        c = color;
                    } else {
                        c = bgcolor;
                    }

                    if (c != C565_TRANSPARENT) {
                        uint16_t ix = ((dy * width) + px); // >> buffer->log2_pixels_per_byte;
                        *(this->curr_buffer + ix) = c;
                    }
                }
            }
        } else {

            data = data + (((ch - font->start_char) * font->width) << incr);

            // Commas and periods don't look good with monospaced fonts. So we remove
            // one blank column from each side
            uint8_t j1, jn;
            if (trim_enabled && (ch == ',' || ch == '.' || ch == ':' || ch == ' ')) {
                j1 = font->trim_punct_start;
                jn = font->width - font->trim_punct_end;
            } else {
                j1 = 0;
                jn = font->width;
            }

            for (j = j1, px = x; j < jn; j++, px++) {

                b = font->size == 2 ? ((uint16_t *)data)[j] : ((uint8_t *)data)[j << incr];

                for (i = si, dy = sdy; i < ei; i++, dy++) {

                    if ((b >> i) & 1) {
                        c = color;
                    } else {
                        c = bgcolor;
                    }

                    if (c != C565_TRANSPARENT) {
                        uint16_t ix = ((dy * width) + px); // >> buffer->log2_pixels_per_byte;
                        *(this->curr_buffer + ix) = c;
                    }
                }
            }
        }
    }
}

uint32_t Display::get_punctuation_width(const FontDef *font_ptr) {
    if (!font_ptr) {
        font_ptr = font;
    }
    return font_ptr->width - font_ptr->trim_punct_end - font_ptr->trim_punct_start;
}

bool Display::is_punctuation(char c) {
    return (c == ',' || c == '.' || c == ':' || c == ' ');
}

void Display::writeString(int16_t x, int16_t y, const char *str, const FontDef *font, uint16_t color, uint16_t bgcolor) {
    // select();

    int delta_punct = get_punctuation_width(font);
    uint16_t max_width = this->hasOffset() ? ow : this->curr_area->box.width;
    uint16_t max_height = this->hasOffset() ? oh : this->curr_area->box.height;

    while (*str) {
        if (*str == '\n' || x + font->width > max_width) {

            y += (font->height + (this->verticalSpacing * 2));
            x = padding_x;

            if (*str == '\n') {
                str++;
            }

            if (y + font->height >= max_height) {
                // No more vertical space
                break;
            }

            while (*str == ' ') {
                // skip spaces at the beginning of the new line
                str++;
            }

            if (!*str) {
                break;
            }
        }

        writeChar(x, y, *str, font, color, bgcolor);

        uint8_t delta_x;
        if (trim_enabled && (*str == ',' || *str == '.' || *str == ':' || *str == ' ')) {
            delta_x = delta_punct;
        } else {
            delta_x = font->width;
        }

        x += delta_x;
        str++;
    }
    px = x;
    py = y;
    // Unselect();
}

void Display::set_trim_enabled(bool b) {
    trim_enabled = b;
}
bool Display::get_trim_enabled() {
    return trim_enabled;
}

uint16_t *Display::getBuffer() {
    return this->curr_buffer;
}

void Display::gotoXY(int16_t x, int16_t y) {
    px = x;
    py = y;
}

uint16_t Display::getY() {
    return py;
}

void Display::gotoCharXY(int16_t x, int16_t y) {
    px = this->padding_x + (x * font->width);
    py = this->padding_y + this->verticalSpacing + (y * (font->height + (this->verticalSpacing * 2)));
}

uint8_t Display::getVerticalLineSpacing() {
    return this->verticalSpacing;
}

void Display::setVerticalLineSpacing(uint8_t pixels) {
    py = (int)py - ((int)this->verticalSpacing - (int)pixels);
    this->verticalSpacing = pixels;
}

void Display::setColor(uint16_t c) {
    this->color = c;
}

void Display::setBgColor(uint16_t c) {
    this->bgColor = c;
}

size_t Display::write(const uint8_t *buffer, size_t) {
    writeString(px, py, (const char *)buffer, font, color, bgColor);
    return 0;
}

size_t Display::write(uint8_t c) {
    writeChar(px, py, c, font, color, bgColor);
    return 0;
}

void Display::setFont(const FontDef *f) {
    this->font = f;
}

const FontDef *Display::getFont(void) {
    return this->font;
}

void Display::test(void) {
}

size_t Display::print(const char str[]) {
    return write(str);
}

size_t Display::print(const char str[], const char *value, const char units[]) {

    return this->print(str, value, units, C565_WHITE, C565_CYAN_DARK, C565_GREY_LIGHT);
}

size_t Display::print(const char str[], const char *value, const char units[], uint16_t labelColor, uint16_t valueColor, uint16_t unitsColor) {

    uint16_t c = this->color;
    this->setColor(labelColor);
    write(str);
    this->setColor(valueColor);
    write(value);
    this->setColor(unitsColor);
    write(units);
    this->setColor(c);

    return 0;
}

void Display::set_padding(uint16_t x, uint16_t y) {
    padding_x = x;
    padding_y = y;
}

uint16_t Display::get_padding_x() {
    return padding_x;
}

uint16_t Display::get_padding_y() {
    return padding_y;
}

size_t Display::print(char c) {
    return write(c);
}

size_t Display::print(unsigned char b, int base) {
    return print((unsigned long)b, base);
}

size_t Display::print(int n, int base) {
    return print((long)n, base);
}

size_t Display::print(unsigned int n, int base) {
    return print((unsigned long)n, base);
}

size_t Display::print(long n, int base) {
    if (base == 0) {
        return write(n);
    } else if (base == 10) {
        if (n < 0) {
            int t = print('-');
            n = -n;
            return print_number(n, 10) + t;
        }
        return print_number(n, 10);
    } else {
        return print_number(n, base);
    }
}

size_t Display::print(unsigned long n, int base) {
    if (base == 0) {
        return write(n);
    } else {
        return print_number(n, base);
    }
}

size_t Display::print(double n, int digits) {
    return print_float(n, digits);
}

size_t Display::print_number(unsigned long n, uint8_t base) {
    char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
    char *str = &buf[sizeof(buf) - 1];

    *str = '\0';

    // prevent crash if called with base == 1
    if (base < 2) {
        base = 10;
    }

    do {
        unsigned long m = n;
        n /= base;
        char c = m - base * n;
        *--str = c < 10 ? c + '0' : c + 'A' - 10;
    } while (n);

    return write(str);
}

size_t Display::print_float(double number, uint8_t digits) {
    size_t n = 0;

    if (isnan(number)) {
        return print("nan");
    }
    if (isinf(number)) {
        return print("inf");
    }
    if (number > 4294967040.0) {
        return print("ovf"); // constant determined empirically
    }
    if (number < -4294967040.0) {
        return print("ovf"); // constant determined empirically
    }

    // Handle negative numbers
    if (number < 0.0) {
        n += print('-');
        number = -number;
    }

    // Round correctly so that print(1.999, 2) prints as "2.00"
    double rounding = 0.5;
    for (uint8_t i = 0; i < digits; ++i) {
        rounding /= 10.0;
    }

    number += rounding;

    // Extract the integer part of the number and print it
    unsigned long int_part = (unsigned long)number;
    double remainder = number - (double)int_part;
    n += print(int_part);

    // Print the decimal point, but only if there are digits beyond
    if (digits > 0) {
        n += print(".");
    }

    // Extract digits from the remainder one at a time
    while (digits-- > 0) {
        remainder *= 10.0;
        int toPrint = int(remainder);
        n += print(toPrint);
        remainder -= toPrint;
    }

    return n;
}

bool Display::get_wrap_text() const {
    return wrap_text;
}

void Display::set_wrap_text(bool wrap_text) {
    Display::wrap_text = wrap_text;
}

std::string Display::fit_text(const std::string &text, int max_width, int max_height) {
    int mw = (max_width < 0 ? curr_area->box.width : max_width);
    int max_chars_per_line = mw / font->width;
    int max_lines = (max_height < 0 ? curr_area->box.height : max_height) / font->height;
    const char *ellipsis{" <...> "};
    int ellipsis_length = strlen(ellipsis);

    if (max_chars_per_line <= ellipsis_length || max_lines <= 0) {
        return "!!!";
    }

    std::string result;
    size_t start = 0;
    int line_count = 0;

    while (start < text.length() && line_count < max_lines) {
        size_t end = text.find('\n', start);
        if (end == std::string::npos) {
            end = text.length();
        }

        std::string line = text.substr(start, end - start);

        // Truncate line if too long
        if (get_text_size(line).width() > mw) {
            int truncate_at = (max_chars_per_line - ellipsis_length) / 2;
            if (truncate_at > 0) {
                line = line.substr(0, truncate_at) + ellipsis + line.substr(line.length() - truncate_at);
            } else {
                line = line.substr(0, max_chars_per_line);
            }
        }

        result += line;
        line_count++;

        // Add newline if not the last line and we haven't hit the limit
        if (end < text.length() && line_count < max_lines) {
            result += '\n';
        }

        start = end + 1;
    }

    return result;
}

Size Display::get_text_size(const std::string &text) {
    return get_text_size(text.c_str());
}

Size Display::get_text_size(const char *text, const FontDef *f) {

    if (!f) {
        f = font;
    }

    if (!text[0]) {
        return {0, 0};
    }

    int max_width = 0;
    int current_width = 0;
    int line_count = 1; // Start with 1 line
    int i = 0;
    char c = text[i];
    while (c) {
        if (c == '\n') {
            // End of line - update max width and start new line
            max_width = std::max(max_width, current_width);
            current_width = 0;
            line_count++;
        } else if (is_punctuation(c) && trim_enabled) {
            current_width += get_punctuation_width();
        } else {
            // Regular character
            current_width += f->width;
        }

        c = text[++i];
    }

    // Don't forget the last line if it doesn't end with newline
    max_width = std::max(max_width, current_width);

    return {max_width, line_count * (f->height + getVerticalLineSpacing() * 2)};
}
// Helper function to extract RGB components from RGB565 format
static inline void extract_rgb565(uint16_t pixel, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (pixel >> 11) & 0x1F; // Extract 5-bit red
    *g = (pixel >> 5) & 0x3F;  // Extract 6-bit green
    *b = pixel & 0x1F;         // Extract 5-bit blue
}

// Helper function to combine RGB components into RGB565 format
static inline uint16_t combine_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

// Function to blend two RGB565 pixels with a given alpha value (0-255)
static inline uint16_t blend_pixel(uint16_t fg_pixel, uint16_t bg_pixel, uint8_t alpha) {
    uint8_t fg_r, fg_g, fg_b;
    uint8_t bg_r, bg_g, bg_b;

    // Extract RGB components from foreground and background pixels
    extract_rgb565(fg_pixel, &fg_r, &fg_g, &fg_b);
    extract_rgb565(bg_pixel, &bg_r, &bg_g, &bg_b);

    // Scale alpha to a fixed-point range [0, 256] to avoid division by 255
    uint16_t alpha_scaled = (alpha + 1); // Approximate (alpha / 255) as (alpha + 1) / 256

    // Blend using fixed-point arithmetic
    uint8_t blended_r = ((fg_r * alpha_scaled + bg_r * (256 - alpha_scaled)) >> 8);
    uint8_t blended_g = ((fg_g * alpha_scaled + bg_g * (256 - alpha_scaled)) >> 8);
    uint8_t blended_b = ((fg_b * alpha_scaled + bg_b * (256 - alpha_scaled)) >> 8);

    // Combine blended RGB components back into RGB565 format
    return combine_rgb565(blended_r, blended_g, blended_b);
}
