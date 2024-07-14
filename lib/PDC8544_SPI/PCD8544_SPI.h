
#pragma once

#include <stdint-gcc.h>
#include <string.h>
#include <stm32f4xx.h>

#define ST7789_SPI_PORT hspi1
extern SPI_HandleTypeDef ST7789_SPI_PORT;


#define PCD8544_GPIO_PORT GPIOB
#define PCD8544_RST_PIN GPIO_PIN_0
#define	PCD8544_DC_PIN GPIO_PIN_2
#define PCD8544_CS_PIN GPIO_PIN_1

#define PCD8544_COMMAND		0
#define PCD8544_DATA		1
#define PCD8544_CS_DC_PIN		PCD8544_DC_PIN |  PCD8544_CS_PIN

/*

#define PCD8544_PORT		PORTD
#define PCD8544_DDR			DDRD	// Should be DDRx, x = port name (B, C, D, etc.)

#define PIN_DC				0x80	// D7 / PD7
#define PIN_RESET			0x20	// D5 / PD5
#define PIN_CE				0x40	// D6 / PD6
#define PINS_CE_DC			(PIN_DC | PIN_CE)

// When DC is '1' the LCD expects data, when it is '0' it expects a command.

*/
// You may find a different size screen, but this one is 84 by 48 pixels
#define DISPLAY_X_PIXELS	84
#define DISPLAY_Y_PIXELS	48
#define PCD8544_ROWS		6

#define BUF_LEN				DISPLAY_X_PIXELS * PCD8544_ROWS // 84 * 6 (6 rows of 8 bits)

// Functions gotoXY, writeBitmap, renderString, writeLine and writeRect
// will return PCD8544_SUCCESS if they succeed and PCD8544_ERROR if they fail.
#define PCD8544_SUCCESS		1
#define PCD8544_ERROR		0

enum FONT {
	FONT5X8,FONT3X5
};


class PCD8544_SPI_FB
{
public:

	PCD8544_SPI_FB();

	// Call a render method after any print/write methods are called.
	// For best perofrmance aggragate all writes before calling a render method.
	void renderAll();
	uint8_t renderString(uint8_t x, uint8_t y, uint16_t length);

	void setPixel(uint8_t x, uint8_t y, uint8_t value);

	// WriteLine currently only supports horizontal and vertical lines.
	uint8_t writeLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
	uint8_t writeRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool fill = false);

	void begin(bool invert = false);
	void begin(bool invert, uint8_t vop, uint8_t tempCoef, uint8_t bias);
	void clear(bool render = true);

	uint8_t gotoXY(uint8_t x, uint8_t y);
	virtual size_t write(uint8_t uint8_t);
	uint8_t writeBitmap(const uint8_t *bitmap, uint8_t x, uint8_t y, uint8_t width, uint8_t height);



    size_t write(const char *str) {
        if(str == NULL)
            return 0;
        return write((const uint8_t *) str, strlen(str));
    }
    virtual size_t write(const uint8_t *buffer, size_t size);
    size_t write(const char *buffer, size_t size) {
        return write((const uint8_t *) buffer, size);
    }
    // These handle ambiguity for write(0) case, because (0) can be a pointer or an integer
    inline size_t write(short t) { return write((uint8_t)t); }
    inline size_t write(unsigned short t) { return write((uint8_t)t); }
    inline size_t write(int t) { return write((uint8_t)t); }
    inline size_t write(unsigned int t) { return write((uint8_t)t); }
    inline size_t write(long t) { return write((uint8_t)t); }
    inline size_t write(unsigned long t) { return write((uint8_t)t); }
    // Enable write(char) to fall through to write(uint8_t)
    inline size_t write(char c) { return write((uint8_t) c); }
    inline size_t write(int8_t c) { return write((uint8_t) c); }


    size_t print(const char[]);
    size_t print(char);
    size_t print(unsigned char, int = 10);
    size_t print(int, int = 10);
    size_t print(unsigned int, int = 10);
    size_t print(long, int = 10);
    size_t print(unsigned long, int = 10);
    size_t print(double, int = 2);

private:

	//void init(void);
	void writeLcd(uint8_t dataOrCommand, uint8_t data);
	void writeLcd(uint8_t dataOrCommand, uint8_t *data, uint16_t count);

    size_t printNumber(unsigned long, uint8_t);
    size_t printFloat(double, uint8_t);

	inline void swap(uint8_t &a, uint8_t &b);
	uint16_t m_Position;
	uint8_t m_Buffer[BUF_LEN];
};

//This table contains the hex values that represent pixels
//for a font that is 5 pixels wide and 8 pixels high
static const  uint8_t ASCII[] = {
        0x00, 0x00, 0x00, 0x00,                  // Code for char
        0x2F, 0x00, 0x00, 0x00,                  // Code for char !
        0x03, 0x00, 0x03, 0x00,                  // Code for char "
        0x14, 0x3E, 0x3E, 0x14,                  // Code for char #
        0x2E, 0x6A, 0x2B, 0x3A,                  // Code for char $
        0x36, 0x1A, 0x2C, 0x36,                  // Code for char %
        0x1C, 0x17, 0x15, 0x34,                  // Code for char &
        0x03, 0x00, 0x00, 0x00,                  // Code for char '
        0x1E, 0x21, 0x00, 0x00,                  // Code for char (
        0x21, 0x1E, 0x00, 0x00,                  // Code for char )
        0x2A, 0x1C, 0x1C, 0x2A,                  // Code for char *
        0x08, 0x1C, 0x08, 0x00,                  // Code for char +
        0x40, 0x20, 0x00, 0x00,                  // Code for char ,
        0x08, 0x08, 0x00, 0x00,                  // Code for char -
        0x20, 0x00, 0x00, 0x00,                  // Code for char .
        0x20, 0x10, 0x08, 0x04,                  // Code for char /
        0x3F, 0x21, 0x21, 0x3F,                  // Code for char 0
        0x01, 0x3F, 0x00, 0x00,                  // Code for char 1
        0x39, 0x29, 0x29, 0x2F,                  // Code for char 2
        0x29, 0x29, 0x29, 0x3F,                  // Code for char 3
        0x0E, 0x08, 0x08, 0x3E,                  // Code for char 4
        0x2F, 0x29, 0x29, 0x39,                  // Code for char 5
        0x3F, 0x29, 0x29, 0x39,                  // Code for char 6
        0x01, 0x39, 0x05, 0x03,                  // Code for char 7
        0x3F, 0x29, 0x29, 0x3F,                  // Code for char 8
        0x2F, 0x29, 0x29, 0x3F,                  // Code for char 9
        0x28, 0x00, 0x00, 0x00,                  // Code for char :
        0x40, 0x28, 0x00, 0x00,                  // Code for char ;
        0x08, 0x14, 0x22, 0x00,                  // Code for char <
        0x14, 0x14, 0x00, 0x00,                  // Code for char =
        0x22, 0x14, 0x08, 0x00,                  // Code for char >
        0x01, 0x2D, 0x05, 0x07,                  // Code for char ?
        0x3F, 0x21, 0x3D, 0x3F,                  // Code for char @
        0x3E, 0x09, 0x09, 0x3E,                  // Code for char A
        0x3F, 0x29, 0x29, 0x1E,                  // Code for char B
        0x1E, 0x21, 0x21, 0x21,                  // Code for char C
        0x3F, 0x21, 0x21, 0x1E,                  // Code for char D
        0x3F, 0x29, 0x29, 0x29,                  // Code for char E
        0x3F, 0x09, 0x09, 0x09,                  // Code for char F
        0x1E, 0x21, 0x29, 0x39,                  // Code for char G
        0x3E, 0x08, 0x08, 0x3E,                  // Code for char H
        0x21, 0x3F, 0x21, 0x00,                  // Code for char I
        0x19, 0x21, 0x1F, 0x00,                  // Code for char J
        0x3E, 0x08, 0x08, 0x37,                  // Code for char K
        0x3F, 0x20, 0x20, 0x20,                  // Code for char L
        0x3F, 0x01, 0x3F, 0x3E,                  // Code for char M
        0x3F, 0x04, 0x08, 0x3F,                  // Code for char N
        0x1E, 0x21, 0x21, 0x1E,                  // Code for char O
        0x3F, 0x09, 0x09, 0x06,                  // Code for char P
        0x1E, 0x21, 0x31, 0x3E,                  // Code for char Q
        0x3F, 0x09, 0x19, 0x26,                  // Code for char R
        0x22, 0x25, 0x29, 0x11,                  // Code for char S
        0x01, 0x3F, 0x01, 0x00,                  // Code for char T
        0x1F, 0x20, 0x20, 0x1F,                  // Code for char U
        0x3F, 0x10, 0x08, 0x07,                  // Code for char V
        0x3F, 0x3F, 0x20, 0x3F,                  // Code for char W
        0x37, 0x08, 0x08, 0x37,                  // Code for char X
        0x27, 0x18, 0x08, 0x07,                  // Code for char Y
        0x31, 0x29, 0x25, 0x23,                  // Code for char Z
        0x3F, 0x21, 0x00, 0x00,                  // Code for char [
        0x20, 0x10, 0x08, 0x04,                  // Code for char BackSlash
        0x21, 0x3F, 0x00, 0x00,                  // Code for char ]
        0x02, 0x01, 0x01, 0x02,                  // Code for char ^
        0x20, 0x20, 0x00, 0x00,                  // Code for char _
        0x01, 0x02, 0x00, 0x00,                  // Code for char `
        0x38, 0x24, 0x24, 0x3C,                  // Code for char a
        0x3F, 0x24, 0x24, 0x18,                  // Code for char b
        0x18, 0x24, 0x24, 0x24,                  // Code for char c
        0x18, 0x24, 0x24, 0x1F,                  // Code for char d
        0x3C, 0x34, 0x2C, 0x20,                  // Code for char e
        0x04, 0x3F, 0x05, 0x00,                  // Code for char f
        0x98, 0xA4, 0xA4, 0x78,                  // Code for char g
        0x3F, 0x04, 0x04, 0x38,                  // Code for char h
        0x3D, 0x00, 0x00, 0x00,                  // Code for char i
        0x80, 0xFD, 0x00, 0x00,                  // Code for char j
        0x3F, 0x10, 0x10, 0x2C,                  // Code for char k
        0x3F, 0x00, 0x00, 0x00,                  // Code for char l
        0x3C, 0x04, 0x3C, 0x3C,                  // Code for char m
        0x3C, 0x04, 0x04, 0x38,                  // Code for char n
        0x18, 0x24, 0x24, 0x18,                  // Code for char o
        0xFC, 0x24, 0x24, 0x18,                  // Code for char p
        0x18, 0x24, 0x24, 0xFC,                  // Code for char q
        0x3C, 0x08, 0x04, 0x00,                  // Code for char r
        0x24, 0x2A, 0x2A, 0x12,                  // Code for char s
        0x04, 0x3F, 0x24, 0x00,                  // Code for char t
        0x1C, 0x20, 0x20, 0x1C,                  // Code for char u
        0x0C, 0x30, 0x10, 0x0C,                  // Code for char v
        0x3C, 0x20, 0x38, 0x3C,                  // Code for char w
        0x2C, 0x10, 0x10, 0x2C,                  // Code for char x
        0xBC, 0xA0, 0xA0, 0x7C,                  // Code for char y
        0x24, 0x34, 0x2C, 0x24,                  // Code for char z
        0x08, 0x33, 0x21, 0x00,                  // Code for char {
        0x3F, 0x00, 0x00, 0x00,                  // Code for char |
        0x21, 0x33, 0x08, 0x00,                  // Code for char }
        0x01, 0x02, 0x02, 0x01,                  // Code for char ~
        0x1F, 0x1F, 0x00, 0x00,                   // Code for char 
        0xfe, 0xfe, 0xfe, 0xfe                   // Code for bar 
};
