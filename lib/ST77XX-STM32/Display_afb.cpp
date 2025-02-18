#include "Display_afb.h"
#include <string.h>
#include <math.h>
#include <hw/stm32.h>
#include <stdint.h>
#include <algorithm>
#include "ILI9341_fb.h"
#include "Painter.hpp"
#include "ui/ui_types.h"

#define min2(a, b) ((a) < (b) ? (a) : (b))
#define max2(a, b) ((a) > (b) ? (a) : (b))

#define SETPIXEL(x, y, c) (*(this->curr_buffer + x + (y >> 16)) = c)

/* RGB565 buffer for transferring pixels to the display using DMA */
static const uint16_t b565_buffer_size = DISPLAY_TOTAL_WIDTH * 16;

uint16_t b565_buffer[b565_buffer_size] __attribute__((aligned(4)));

Display::Display(SPI_HandleTypeDef *spi_port) {
    this->spi_port = spi_port;
    this->curr_buffer = b565_buffer;
}

void Display::convertPalette888to565(const uint32_t *orig, uint16_t *dest, uint8_t size) {
    for (int i = 0; i < size; i++) {
        dest[i] = SWAP_BYTES(RGB888_TO_RGB565(orig[i]));
    }
}

void Display::set_transparency(uint8_t v) { transparency = v; }

uint8_t Display::get_transparency() { return transparency; }

void Display::clear(uint16_t color) {

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

bool Display::hasOffset() { return ow; }

Box Display::getOffset() { return {ox, oy, ow, oh}; }

void Display::clearOffset() {
    ox = 0;
    oy = 0;
    ow = 0;
    oh = 0;
}

void Display::setEnabled(bool b) { this->enabled = b; }

bool Display::getEnabled() { return this->enabled; }

void Display::drawArea(Area *area, Painter *painter) { drawArea(area, painter, true); }

void Display::drawArea(Area *area, Painter *painter, bool pad_display) {

    if (this->enabled) {

        this->drawing = true;

        this->curr_area = area;

        this->gotoXY(0, 0);

        uint16_t x = area->box.x;
        uint16_t y = area->box.y;

        if (pad_display) {
            x += DISPLAY_PADDING;
            y += DISPLAY_PADDING;
        }

        // Number of pixels in the area
        uint16_t buffer_size_pixels_remaining = area->size;

        // Number of bytes of the block sent to the driver each transfer. This is limited by the memory available

        // We set the buffer size to be a whole number of lines of the area so we can easily determine whether we can write
        // or not, depending on the relative position of the buffer in the area

        // integer floor of buffer_size/2/width (half buffer lines)
        uint16_t w = area->box.width;

        uint16_t a = b565_buffer_size / 2;
        uint16_t d = a / w;
        this->chunk_height = d * w == a ? d : d - ((a < 0) ^ (w < 0));

        // chunk_height should not be greater than the area height.
        // An area can be small enough (less than half the buffer size) that it can be drawn in a single DMA transfer
        if (this->chunk_height > area->box.height) {
            this->chunk_height = area->box.height;
        }

        uint16_t max_buffer_size = this->chunk_height * w;

        uint16_t dma_buffer_size = buffer_size_pixels_remaining < max_buffer_size ? buffer_size_pixels_remaining : max_buffer_size;

        uint16_t half_dma_buffer_size = max_buffer_size;

        dma_buffer_size *= 2; // we will draw two chunks per DMA transfer

        dma_buffer_size = min2(dma_buffer_size, area->size); // But we won't transfer more than the area size

        uint16_t dma_transfer_length = dma_buffer_size * 2; // 2 bytes per pixel

        setAddressWindow(x, y, x + area->box.width - 1, y + area->box.height - 1);

        if (this->use_dma) {
            InitDisplayDataTransfer();
        } else {
            DISP_DC_PORT->BSRR |= DISP_DC_PIN; // DC PIN SET
        }

        this->current_line = 0;
        // uint16_t dy = area->y;

        this->curr_buffer = b565_buffer;

#if DEBUG_LCD
        char str[10];
        if (area->show_fps) {
            sprintf(str, "%d.%d", (int)area->fps, (int)(area->fps * 10) % 10);
        }
#endif

        while (this->current_line < area->box.height) {

            this->current_last_line = min2(this->current_line + this->chunk_height, area->box.height) - 1;

            // Prevent any interruption of the paint callback
            // NVIC_DisableIRQ(TIM8_TRG_COM_TIM14_IRQn); // Disabled, since I'm checking the 'busy' flag from aoutside
            this->busy = true;

            painter->paint_callback();

            // IMPORTANT: The callback must be executed faster than the half transfer of the DMA buffer. Otherwise, the buffer
            // won't be fully written when the DMA transfers it.
            // (This can be a problem if the compilation is set without speed optimizations and, be aware, these
            // optimizations need sometimes to be turned off to be able to properly debug with breakpoints)

#if DEBUG_LCD
            if (area->show_fps) {
                this->writeString(0, curr_area->box.height - 11, str, (FontDef *)&Font_7x10, C565_BLACK, C565_WHITE);
                this->writeLine(0, curr_area->box.height - 12, 21, curr_area->box.height - 12, C565_WHITE);
            }
#endif
            this->busy = false;
            // NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);

            this->current_line += this->chunk_height;
            // dy += this->chunk_height;

            if (!this->use_dma) {
                // Transfer the buffer without DMA
                // Experimental: Just to see if I manage to share one SPI bus with two devices, one of which transfers within an interrupt
                for (uint32_t i = 0; i < dma_buffer_size; i++) {

                    // HAL_TIM_Base_Stop_IT(&htim15);
                    DISP_CE_PORT->BSRR |= DISP_CE_PIN << 16;

                    if ((spi_port->Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE) {
                        spi_port->Instance->CR1 |= SPI_CR1_SPE; // enable SPI
                    }
                    *(__IO uint8_t *)&spi_port->Instance->DR =
                        *((__IO uint8_t *)this->curr_buffer + i); // Write data to be transmitted to the SPI data register
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

            if (this->curr_buffer == b565_buffer) {

                // The first half of the RGB buffer is ready to be transferred
                // If DMA is ready (second half has been transferred, so state == READY), we start another transfer now

                this->curr_buffer = b565_buffer + half_dma_buffer_size;

                if (this->use_dma) {
                    // GPIOD->BSRR |= GPIO_PIN_5;
                    while (HAL_SPI_GetState(spi_port) != HAL_SPI_STATE_READY) {
                        ;
                    }
                    // GPIOD->BSRR |= GPIO_PIN_5<<16;
                    this->DMAHalfTransferCompleted = false;
                    HAL_SPI_Transmit_DMA(spi_port, ((uint8_t *)b565_buffer), dma_transfer_length);
                }
            } else {

                // The second half of the RGB buffer is ready
                // Wait for the first half of the buffer to be transferred before start again

                this->curr_buffer = b565_buffer;

                if (this->use_dma) {
                    while (!this->DMAHalfTransferCompleted) {
                        ;
                    }
                }

                // The last chunk may need fewer bytes to transfer
                if (area->box.height - this->current_line < this->chunk_height << 1) {
                    dma_buffer_size = (area->box.height - this->current_line) * area->box.width;
                    dma_transfer_length = dma_buffer_size << 1;
                }
            }
        }

        if (this->use_dma) {
            while (HAL_SPI_GetState(spi_port) != HAL_SPI_STATE_READY) {
                ;
            }
            EndDisplayDataTransfer();
        } else {
            DISP_DC_PORT->BSRR |= DISP_DC_PIN << 16; // DC PIN UNSET
        }

        this->drawing = false;
    }
}

// Function to draw a single corner using midpoint circle algorithm
void Display::drawCorner(uint16_t centerX, uint16_t centerY, uint8_t radius, uint8_t quadrant, bool filled) {

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

void Display::drawRoundedRectangle(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, uint16_t radius, bool filled) {
    drawRoundedRectangle(x0, y0, width, height, radius, filled, true, true, true, true);
}

void Display::drawRoundedRectangle(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, uint16_t radius, bool filled, bool top_left, bool top_right,
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
        drawCorner(x0 + width - radius - 1, y0 + radius, radius, 1, filled); // Top-right
    }
    if (top_left) {
        drawCorner(x0 + radius, y0 + radius, radius, 2, filled); // Top-left
    }
    if (bottom_left) {
        drawCorner(x0 + radius, y0 + height - radius - 1, radius, 3, filled); // Bottom-left
    }
    if (bottom_right) {
        drawCorner(x0 + width - radius - 1, y0 + height - radius - 1, radius, 4, filled); // Bottom-right
    }
}

void Display::DMATxHalfCpltCallback(void) {
    this->DMAHalfTransferCompleted = true;
    // GPIOB->BSRR= GPIO_PIN_5 << 16;
}

void Display::DMATxCpltCallback(void) {}

uint16_t Display::getColor() { return this->color; }

void Display::fillBuffer(uint16_t c) {

    if (ow == 0) {
        // No offset, so fill the whole buffer
        for (uint16_t i = 0; i < this->chunk_height * this->curr_area->box.width; i++) {
            *(curr_buffer + i) = c;
        }
    } else {
        // Partial buffer fill
        fill(0, 0, ow - 1, oh - 1, c);
    }
}

void Display::fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t c) {

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

            if (c >> 8 == c && 0x0F) {
                memset(p, c, row_bytes);
            } else {
                std::fill(p, p + (row_bytes >> 1), c);
            }
            p += this->curr_area->box.width;
        }
    }
}

void Display::setPixel(uint16_t x, uint16_t y, uint16_t c) {

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

void Display::writeLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) { writeLine(x1, y1, x2, y2, this->color); }

void Display::writeVertLine(uint16_t x, uint16_t y1, uint16_t y2, uint16_t color) {

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

void Display::writeLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) { this->writeLine(x1, y1, x2, y2, color, 1); }

void Display::writeLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color, uint8_t) {

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

        uint16_t ipx = x1;
        uint16_t ipy = y1;

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

void Display::writeRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    // select();
    writeRect(x1, y1, x2, y2, C565_WHITE);
}

void Display::writeRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    // select();
    writeLine(x1, y1, x2, y1, color);
    writeLine(x1, y1, x1, y2, color);
    writeLine(x1, y2, x2, y2, color);
    writeLine(x2, y1, x2, y2, color);
    // Unselect();
}

void Display::drawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color) {
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

void Display::writeChar(uint16_t x, uint16_t y, char ch, const FontDef *font, uint16_t color, uint16_t bgcolor) {

    x += ox;
    y += oy;

    uint16_t i, b, j;
    int px, py = y, dy, y1, y2;
    uint16_t c;

    uint16_t width = this->curr_area->box.width;

    y1 = this->current_line;
    y2 = this->current_line + this->chunk_height;

    if (py + font->height >= y1 && py < y2) {

        int si = max2(0, y1 - py);
        int ei = min2(font->height, y2 - py);
        int sdy = dy = max2(0, py - this->current_line);
        uint16_t mask;
        const uint8_t *data = ((FontDef8 *)font)->data;

        uint8_t incr = font->size - 1;

        if (font->encoding == ROWS) {

            mask = font->size == 2 ? 0x8000 : 0x80;
            data = data + (((ch - 32) * font->height) << incr);

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

            data = data + (((ch - 32) * font->width) << incr);

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

void Display::writeString(uint16_t x, uint16_t y, const char *str, const FontDef *font, uint16_t color, uint16_t bgcolor) {
    // select();

    uint8_t delta_punct = font->width - font->trim_punct_end - font->trim_punct_start;
    uint16_t max_width = this->hasOffset() ? ow : this->curr_area->box.width;
    uint16_t max_height = this->hasOffset() ? oh : this->curr_area->box.height;

    while (*str) {
        if (*str == '\n' || x + font->width >= max_width) {

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

void Display::set_trim_enabled(bool b) { trim_enabled = b; }

uint16_t *Display::getBuffer() { return this->curr_buffer; }

void Display::gotoXY(uint16_t x, uint16_t y) {
    px = x;
    py = y;
}

void Display::gotoCharXY(uint16_t x, uint16_t y) {
    px = this->padding_x + (x * font->width);
    py = this->verticalSpacing + (y * (font->height + (this->verticalSpacing * 2)));
}

uint8_t Display::getVerticalLineSpacing() { return this->verticalSpacing; }

void Display::setVerticalLineSpacing(uint8_t pixels) {
    py = (int)py - ((int)this->verticalSpacing - (int)pixels);
    this->verticalSpacing = pixels;
}

void Display::setColor(uint16_t c) { this->color = c; }

void Display::setBgColor(uint16_t c) { this->bgColor = c; }

size_t Display::write(const uint8_t *buffer, size_t) {
    writeString(px, py, (const char *)buffer, font, color, bgColor);
    return 0;
}

size_t Display::write(uint8_t c) {
    writeChar(px, py, c, font, color, bgColor);
    return 0;
}

void Display::setFont(const FontDef *f) { this->font = f; }

const FontDef *Display::getFont(void) { return this->font; }

void Display::test(void) {}

size_t Display::print(const char str[]) { return write(str); }

size_t Display::print(const char str[], const char *value, const char units[]) {

    return this->print(str, value, units, C565_GREY_LIGHT, C565_WHITE, C565_GREY_LIGHT);
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

void Display::setPadding(uint16_t x, uint16_t y) {
    padding_x = x;
    padding_y = y;
}

uint16_t Display::get_padding_x() { return padding_x; }

uint16_t Display::get_padding_y() { return padding_y; }

size_t Display::print(char c) { return write(c); }

size_t Display::print(unsigned char b, int base) { return print((unsigned long)b, base); }

size_t Display::print(int n, int base) { return print((long)n, base); }

size_t Display::print(unsigned int n, int base) { return print((unsigned long)n, base); }

size_t Display::print(long n, int base) {
    if (base == 0) {
        return write(n);
    } else if (base == 10) {
        if (n < 0) {
            int t = print('-');
            n = -n;
            return printNumber(n, 10) + t;
        }
        return printNumber(n, 10);
    } else {
        return printNumber(n, base);
    }
}

size_t Display::print(unsigned long n, int base) {
    if (base == 0) {
        return write(n);
    } else {
        return printNumber(n, base);
    }
}

size_t Display::print(double n, int digits) { return printFloat(n, digits); }

size_t Display::printNumber(unsigned long n, uint8_t base) {
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

size_t Display::printFloat(double number, uint8_t digits) {
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

bool Display::getWrapText() const { return wrap_text; }

void Display::setWrapText(bool wrap_text) { Display::wrap_text = wrap_text; }

// Helper function to extract RGB components from RGB565 format
static inline void extract_rgb565(uint16_t pixel, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (pixel >> 11) & 0x1F; // Extract 5-bit red
    *g = (pixel >> 5) & 0x3F;  // Extract 6-bit green
    *b = pixel & 0x1F;         // Extract 5-bit blue
}

// Helper function to combine RGB components into RGB565 format
static inline uint16_t combine_rgb565(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F); }

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
