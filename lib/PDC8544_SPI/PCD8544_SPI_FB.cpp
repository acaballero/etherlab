#include <string.h>
#include <math.h>
#include "PCD8544_SPI.h"


PCD8544_SPI_FB::PCD8544_SPI_FB()
{
}

void PCD8544_SPI_FB::begin(bool invert)
{
	this->begin(invert, 0xB0, 0x04, 0x12);
}

void PCD8544_SPI_FB::begin(bool invert, uint8_t vop, uint8_t tempCoef, uint8_t bias)
{
    HAL_GPIO_WritePin(PCD8544_GPIO_PORT,PCD8544_RST_PIN|PCD8544_DC_PIN|PCD8544_CS_PIN,GPIO_PIN_SET);

	// LCD init section:
	
	uint8_t invertSetting = invert ? 0x0D : 0x0C;
	// Must reset LCD first!
    HAL_GPIO_WritePin(PCD8544_GPIO_PORT,PCD8544_RST_PIN,GPIO_PIN_RESET);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(PCD8544_GPIO_PORT,PCD8544_RST_PIN,GPIO_PIN_SET);

    HAL_GPIO_WritePin(PCD8544_GPIO_PORT,PCD8544_CS_PIN,GPIO_PIN_RESET);

	this->writeLcd(PCD8544_COMMAND, 0x21); //Tell LCD that extended commands follow
	this->writeLcd(PCD8544_COMMAND, vop); //Set LCD Vop (Contrast): Try 0xB1(good @ 3.3V) or 0xBF if your display is too dark
	this->writeLcd(PCD8544_COMMAND, tempCoef); //Set Temp coefficent
	this->writeLcd(PCD8544_COMMAND, bias); //LCD bias mode 1:48: Try 0x13 or 0x14. Mine works best with 1:65/1:65

	this->writeLcd(PCD8544_COMMAND, 0x20); //We must send 0x20 before modifying the display control mode
	this->writeLcd(PCD8544_COMMAND, invertSetting); //Set display control, normal mode. 0x0D for inverse

	this->clear(false);
}


/* default implementation: may be overridden */
size_t PCD8544_SPI_FB::write(const uint8_t *buffer, size_t size) {


    size_t n = 0;
    while (size--) {
        size_t ret = write(*(buffer++));
        if (ret == 0) {
            // Write of last byte didn't complete, abort additional processing
            break;
        }
        n += ret;
    }
    return n;
}

size_t PCD8544_SPI_FB::write(uint8_t data)
{
	// Non-ASCII characters are not supported.
	if (data < 0x20 || data > 0x80) return 0;

	if (this->m_Position + 4 >= BUF_LEN) this->m_Position -= (BUF_LEN - 5);

	memcpy(this->m_Buffer+this->m_Position,ASCII+((data - 0x20)*4), 4);

	this->m_Buffer[this->m_Position+4] = 0x00;

	this->m_Position += 5;
	if (this->m_Position >= BUF_LEN) this->m_Position -= BUF_LEN;
	//this->m_Position %= BUF_LEN;
	return 1;
}

void PCD8544_SPI_FB::clear(bool render)
{
	memset(this->m_Buffer, 0x00, sizeof(this->m_Buffer));
	if (render)
		this->renderAll();
	this->gotoXY(0, 0);
}

uint8_t PCD8544_SPI_FB::gotoXY(uint8_t x, uint8_t y)
{	
	if (x >= DISPLAY_X_PIXELS || y >= PCD8544_ROWS) return PCD8544_ERROR;
//	this->writeLcd(PCD8544_COMMAND, 0x80 | x);  // Column.
//	this->writeLcd(PCD8544_COMMAND, 0x40 | y);  // Row.
	this->m_Position = (DISPLAY_X_PIXELS * y) + x;
	return PCD8544_SUCCESS;
	return PCD8544_SUCCESS;
}

uint8_t PCD8544_SPI_FB::writeBitmap(const uint8_t *bitmap, uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
	//if (x >= DISPLAY_X_PIXELS || y >= DISPLAY_Y_PIXELS) return;
	//this->gotoXY(x, y);
	//uint16_t pos = this->m_Position;
	//for (uint8_t y = 0; y < height; y++)
	//{
	//	memcpy(this->m_Buffer + pos, bitmap + (y*width), width);
	//	pos += DISPLAY_X_PIXELS;
	//}
	
	if (this->gotoXY(x, y) == PCD8544_ERROR) return PCD8544_ERROR;

	uint8_t *pos = this->m_Buffer + this->m_Position;
    const uint8_t *maxY = bitmap + height * width;	

	for (const uint8_t *y = (uint8_t*) bitmap; y < maxY; y += width)
	{
		memcpy(pos, y, width);
		pos += DISPLAY_X_PIXELS;
	}
	return PCD8544_SUCCESS;
}

//void PCD8544_SPI_FB::init(void) 
//{
//	// Must reset LCD first!
//	PCD8544_PORT &= ~PIN_RESET;
//	PCD8544_PORT |= PIN_RESET;
//
//	this->writeLcd(PCD8544_COMMAND, 0x21); //Tell LCD that extended commands follow
//	this->writeLcd(PCD8544_COMMAND, 0xB0); //Set LCD Vop (Contrast): Try 0xB1(good @ 3.3V) or 0xBF if your display is too dark
//	this->writeLcd(PCD8544_COMMAND, 0x04); //Set Temp coefficent
//	this->writeLcd(PCD8544_COMMAND, 0x12); //LCD bias mode 1:48: Try 0x13 or 0x14. Mine works best with 1:65/1:65
//
//	this->writeLcd(PCD8544_COMMAND, 0x20); //We must send 0x20 before modifying the display control mode
//	this->writeLcd(PCD8544_COMMAND, 0x0C); //Set display control, normal mode. 0x0D for inverse
//}

void PCD8544_SPI_FB::renderAll()
{	
	//this->gotoXY(0, 0);
    this->writeLcd(PCD8544_COMMAND, 0x80 | 0);  // Column.
	this->writeLcd(PCD8544_COMMAND, 0x40 | 0);  // Row.
	this->writeLcd(PCD8544_DATA, this->m_Buffer, BUF_LEN);
}

uint8_t PCD8544_SPI_FB::renderString(uint8_t x, uint8_t y, uint16_t length)
{
	if (this->gotoXY(x, y) == PCD8544_ERROR) return PCD8544_ERROR;
	length *= 6;
	this->writeLcd(PCD8544_DATA, this->m_Buffer + this->m_Position, length);
	this->m_Position += length;
	return PCD8544_SUCCESS;
}

void PCD8544_SPI_FB::setPixel(uint8_t x, uint8_t y, uint8_t value)
{
	if (x >= DISPLAY_X_PIXELS || y >= DISPLAY_Y_PIXELS) return;
	uint8_t bank = y / 8;
	uint8_t bitMask = 1 << (y % 8);
	uint8_t &byte = this->m_Buffer[(DISPLAY_X_PIXELS * bank) + x];
	if (value)
		byte |= bitMask;
	else
		byte &= ~bitMask;
}

uint8_t PCD8544_SPI_FB::writeLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
	if (x1 == x2 || y1 == y2)
	{
		if (y1 > y2)
			swap(y1, y2);
		if (x1 > x2)
			swap(x1, x2);
		return this->writeRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
	}
	else
		return PCD8544_ERROR;
}

void PCD8544_SPI_FB::swap(uint8_t &a, uint8_t &b)
{
	uint8_t temp = a;
	a = b;
	b = temp;
}

uint8_t PCD8544_SPI_FB::writeRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool fill)
{
	if (x >= DISPLAY_X_PIXELS || y >= DISPLAY_Y_PIXELS || width == 0 || height == 0) return PCD8544_ERROR;

	// Calculate the bitmasks for the pixels.
	uint8_t bottom = y + height - 1;
	uint8_t bankTop = y / 8;
	uint8_t bankBottom = bottom / 8;
	uint8_t bitMaskTop = 0x01;
	uint8_t bitMaskBottom = 0x80;
	uint8_t bitMaskTopFill = 0xFF;
	uint8_t bitMaskBottomFill = 0xFF;
	bitMaskTop <<= (y % 8);
	bitMaskBottom >>= 7 - (bottom % 8);
	bitMaskTopFill <<= (y % 8);
	bitMaskBottomFill >>= 7 - (bottom % 8);

	// When fill is selected, we'll use the FillMask.
	if (fill)
	{
		bitMaskTop = bitMaskTopFill;
		bitMaskBottom = bitMaskBottomFill;
	}

	// When the rectangle fits in a single bank, we AND the top and bottom masks
	// So that we only fill the required area on the LCD.
	if (bankTop == bankBottom)
	{
		bitMaskTop = fill ? bitMaskTop & bitMaskBottom : bitMaskTop | bitMaskBottom;
		bitMaskTopFill &= bitMaskBottomFill;
	}
	this->gotoXY(x, bankTop);

	// Write the left 'side' of the rectangle on the top bank.
	this->m_Buffer[this->m_Position++] |= bitMaskTopFill;
	// Write a line or a fill.
	for (uint8_t i = 1; i < width-1; i++)
		this->m_Buffer[this->m_Position++] |= bitMaskTop;
	// Write the right 'side' of the rectangle on the top bank.
	if (width > 1)
		this->m_Buffer[this->m_Position++] |= bitMaskTopFill;

	this->m_Position += (DISPLAY_X_PIXELS - width);

	// Write a fill across the middle banks or two sides of the rectangle.
	if (bankBottom - bankTop > 1)
	{
			for (uint8_t i = bankTop + 1; i < bankBottom; i++)
			{
				if (fill)
					memset(this->m_Buffer + this->m_Position, 0xFF, width);
				else
				{
					this->m_Buffer[this->m_Position] = 0xFF;
					this->m_Buffer[this->m_Position+width-1] = 0xFF;
				}
				this->m_Position += DISPLAY_X_PIXELS;
			}
	}
	// If the rectangle spans across more than one bank,
	// apply the same logic for the bottom as the top.
	if (bankBottom > bankTop)
	{
		this->m_Buffer[this->m_Position++] |= bitMaskBottomFill;
		for (uint8_t i = 1; i < width-1; i++)
			this->m_Buffer[this->m_Position++] |= bitMaskBottom;
		if (width > 1)
			this->m_Buffer[this->m_Position++] |= bitMaskBottomFill;
	}
	return PCD8544_SUCCESS;
}

void PCD8544_SPI_FB::writeLcd(uint8_t dataOrCommand,  uint8_t *data, uint16_t count)
{


    HAL_GPIO_WritePin(PCD8544_GPIO_PORT, PCD8544_CS_PIN,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PCD8544_GPIO_PORT, PCD8544_DC_PIN,  dataOrCommand ? GPIO_PIN_SET : GPIO_PIN_RESET);

    for (uint16_t i = count; i > 0; i--)
        HAL_SPI_Transmit(&ST7789_SPI_PORT, &data[count-i], 1, HAL_MAX_DELAY);

    //HAL_SPI_Transmit(&ST7789_SPI_PORT, data, count, HAL_MAX_DELAY);

    HAL_GPIO_WritePin(PCD8544_GPIO_PORT, PCD8544_CS_PIN , GPIO_PIN_SET);
}

void PCD8544_SPI_FB::writeLcd(uint8_t dataOrCommand, uint8_t data)
{
    HAL_GPIO_WritePin(PCD8544_GPIO_PORT, PCD8544_CS_PIN,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PCD8544_GPIO_PORT, PCD8544_DC_PIN,  dataOrCommand ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_SPI_Transmit(&ST7789_SPI_PORT, &data, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(PCD8544_GPIO_PORT, PCD8544_CS_PIN , GPIO_PIN_SET);
}

size_t PCD8544_SPI_FB::print(const char str[]) {
    return write(str);
}

size_t PCD8544_SPI_FB::print(char c) {
    return write(c);
}

size_t PCD8544_SPI_FB::print(unsigned char b, int base) {
    return print((unsigned long) b, base);
}

size_t PCD8544_SPI_FB::print(int n, int base) {
    return print((long) n, base);
}

size_t PCD8544_SPI_FB::print(unsigned int n, int base) {
    return print((unsigned long) n, base);
}

size_t PCD8544_SPI_FB::print(long n, int base) {
    if(base == 0) {
        return write(n);
    } else if(base == 10) {
        if(n < 0) {
            int t = print('-');
            n = -n;
            return printNumber(n, 10) + t;
        }
        return printNumber(n, 10);
    } else {
        return printNumber(n, base);
    }
}

size_t PCD8544_SPI_FB::print(unsigned long n, int base) {
    if(base == 0)
        return write(n);
    else
        return printNumber(n, base);
}

size_t PCD8544_SPI_FB::print(double n, int digits) {
    return printFloat(n, digits);
}


size_t PCD8544_SPI_FB::printNumber(unsigned long n, uint8_t base) {
    char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
    char *str = &buf[sizeof(buf) - 1];

    *str = '\0';

    // prevent crash if called with base == 1
    if(base < 2)
        base = 10;

    do {
        unsigned long m = n;
        n /= base;
        char c = m - base * n;
        *--str = c < 10 ? c + '0' : c + 'A' - 10;
    } while(n);

    return write(str);
}

size_t PCD8544_SPI_FB::printFloat(double number, uint8_t digits) {
    size_t n = 0;

    if(isnan(number))
        return print("nan");
    if(isinf(number))
        return print("inf");
    if(number > 4294967040.0)
        return print("ovf");  // constant determined empirically
    if(number < -4294967040.0)
        return print("ovf");  // constant determined empirically

    // Handle negative numbers
    if(number < 0.0) {
        n += print('-');
        number = -number;
    }

    // Round correctly so that print(1.999, 2) prints as "2.00"
    double rounding = 0.5;
    for(uint8_t i = 0; i < digits; ++i)
        rounding /= 10.0;

    number += rounding;

    // Extract the integer part of the number and print it
    unsigned long int_part = (unsigned long) number;
    double remainder = number - (double) int_part;
    n += print(int_part);

    // Print the decimal point, but only if there are digits beyond
    if(digits > 0) {
        n += print(".");
    }

    // Extract digits from the remainder one at a time
    while(digits-- > 0) {
        remainder *= 10.0;
        int toPrint = int(remainder);
        n += print(toPrint);
        remainder -= toPrint;
    }

    return n;
}