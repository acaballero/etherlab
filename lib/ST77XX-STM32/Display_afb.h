#ifndef __ST77XX_AFB_H
#define __ST77XX_AFB_H

#include "string.h"
#include <stdint.h>

#define DEBUG_LCD 1

#ifndef DISPLAY_X_PIXELS
#if USING_HORIZONAL == 0 || USING_HORIZONAL == 1
#define DISPLAY_PADDING 10
#define DISPLAY_X_PIXELS (480 - (DISPLAY_PADDING << 1))
#define DISPLAY_Y_PIXELS (320 - (DISPLAY_PADDING << 1))
#else
#define DISPLAY_X_PIXELS 240
#define DISPLAY_Y_PIXELS 240
#endif
#endif

#include "Painter.hpp"
#include "ips_font.h"
#include <stm32f4xx.h>

#define ABS(x) ((x) > 0 ? (x) : -(x))

// Reduced color depth params
#define BITS_PER_PIXEL 1 // Default BPP for the pixel buffer
#define PIXELS_PER_BYTE(bpp) (uint8_t)(8 / (1 << (bpp - 1)))

#define BPP1_LOG2_PIXELS_PER_BYTE 3
#define BPP1_PIXEL_MASK 0x0001U

#define BPP2_LOG2_PIXELS_PER_BYTE 2
#define BPP2_PIXEL_MASK 0x0003U

#define BPP4_LOG2_PIXELS_PER_BYTE 1
#define BPP4_PIXEL_MASK 0x000FU

// BGR565
#define C565_BLACK 0x0000
#define C565_DARKEST 0x4208
#define C565_GREY_DARKER 0xC739
#define C565_GREY_DARK 0xEF7B
#define C565_GREY_LIGHT 0xF39C
#define C565_WHITE 0xFFFF
#define C565_NAVY 0x0F00
#define C565_GREEN_DARK 0xE003
#define C565_CYAN_DARK 0xEF03
#define C565_MAROON 0x0078
#define C565_PURPLE 0x0F78
#define C565_OLIVE 0xE07B
#define C565_BLUE 0x1F00
#define C565_GREEN 0xE007
#define C565_CYAN 0xFF07
#define C565_RED 0x00F8
#define C565_MAGENTA 0x1FF8
#define C565_YELLOW 0xE0FF
#define C565_ORANGE 0xA0FD
#define C565_GREENYELLOW 0xE0B7
#define C565_PINK 0x9FFC
#define C565_BROWN 0x609A
#define C565_GOLD 0xA0FE
#define C565_SILVER 0x18C6
#define C565_SKYBLUE 0x7D86
#define C565_VIOLET 0x6828
#define C565_TRANSPARENT 0xFFFE

#define RGB888_TO_RGB565(rgb) ((((rgb >> 19) & 0x1f) << 11) | (((rgb >> 10) & 0x3f) << 5) | (((rgb >> 3) & 0x1f)))

#define SWAP_BYTES(w) (uint16_t)(w >> 8 | w << 8)

struct Area {

    uint16_t x, y;
    uint16_t width, height;
    uint16_t size = 0;
    bool show_fps;
    float fps;
};

class Display {

  public:
    Display(SPI_HandleTypeDef *);

    virtual int16_t begin() = 0;

    virtual int16_t stop() = 0;

    virtual void select() = 0;

    virtual void unselect() = 0;

    virtual void reset() = 0;

    void drawArea(Area *area, Painter *painter);

    void drawArea(Area *, Painter *, bool pad_display);

    uint8_t renderString(uint8_t x, uint8_t y, uint16_t length);

    uint16_t *getBuffer();

    void setPixel(uint16_t x, uint16_t y, uint16_t color);

    void writeVertLine(uint16_t x, uint16_t y1, uint16_t y2, uint16_t color);

    void writeLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

    void writeLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

    void writeLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color, uint8_t width);

    void writeRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

    void writeRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

    void drawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);

    void invertColors(uint8_t invert);

    void test(void);

    void setFont(const FontDef *);

    const FontDef *getFont(void);

    void setColor(uint16_t c);

    void setPadding(uint16_t x, uint16_t y);

    uint16_t get_padding_x();

    uint16_t get_padding_y();

    void setBgColor(uint16_t c);

    uint16_t getColor();

    void clear();

    void gotoXY(uint16_t x, uint16_t y);

    void gotoCharXY(uint16_t x, uint16_t y);

    void fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t c);

    void fillBuffer(uint16_t color);

    void writeChar(uint16_t x, uint16_t y, char ch, const FontDef *font, uint16_t color, uint16_t bgcolor);

    void writeChar(char ch);

    void writeString(uint16_t x, uint16_t y, const char *str, const FontDef *font, uint16_t color, uint16_t bgcolor);

    void drawRoundedRectangle(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, uint16_t radius, bool filled);

    size_t write(uint8_t uint8_t);

    size_t write(const uint8_t *buffer, size_t size);

    size_t write(const char *str) {
        if (str == NULL)
            return 0;
        return write((const uint8_t *)str, strlen(str));
    }

    size_t write(const char *buffer, size_t size) { return write((const uint8_t *)buffer, size); }

    // These handle ambiguity for write(0) case, because (0) can be a pointer or
    // an integer
    inline size_t write(short t) { return write((uint8_t)t); }

    inline size_t write(unsigned short t) { return write((uint8_t)t); }

    inline size_t write(int t) { return write((uint8_t)t); }

    inline size_t write(unsigned int t) { return write((uint8_t)t); }

    inline size_t write(long t) { return write((uint8_t)t); }

    inline size_t write(unsigned long t) { return write((uint8_t)t); }

    // Enable write(char) to fall through to write(uint8_t)
    inline size_t write(char c) { return write((uint8_t)c); }

    inline size_t write(int8_t c) { return write((uint8_t)c); }

    size_t print(const char[]);

    size_t print(const char[], const char *value, const char units[]);

    size_t print(const char[], const char *value, const char units[], uint16_t labelColor, uint16_t valueColor, uint16_t unitsColor);

    size_t print(char);

    size_t print(unsigned char, int = 10);

    size_t print(int, int = 10);

    size_t print(unsigned int, int = 10);

    size_t print(long, int = 10);

    size_t print(unsigned long, int = 10);

    size_t print(double, int = 2);

    void DMATxHalfCpltCallback(void);

    void DMATxCpltCallback(void);

    void setVerticalLineSpacing(uint8_t);

    uint8_t getVerticalLineSpacing();

    void convertPalette888to565(const uint32_t *, uint16_t *, uint8_t size);

    bool getEnabled();

    // Sets the offset box, inside the current area, to which constraint the drawing
    void setOffset(uint16_t, uint16_t, uint16_t, uint16_t);

    void clearOffset();

    void setEnabled(bool);

    bool getWrapText() const;

    void setWrapText(bool wrap_text);

    uint16_t current_line = 0;
    uint16_t chunk_height = 0;
    uint16_t current_last_line = 0;
    volatile bool DMAHalfTransferCompleted = false;
    volatile bool busy = false;
    volatile bool drawing = false;
    bool use_dma = true;

  protected:
    static void fillCallback(Display *, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

    SPI_HandleTypeDef *spi_port;

  private:
    bool enabled = true;
    bool wrap_text = true;
    uint16_t ox = 0; // offset x
    uint16_t oy = 0; // offset y
    uint16_t ow = 0; // offset width
    uint16_t oh = 0; // offset height
    uint16_t px = 0;
    uint16_t py = 0;
    uint16_t padding_x = 0, padding_y = 0;
    const FontDef *font = (FontDef *)&Font_11x18;
    uint16_t color = C565_WHITE;
    uint16_t bgColor = C565_BLACK;
    uint8_t verticalSpacing = 2;
    uint16_t *curr_buffer = 0;
    Area *curr_area = 0;

    virtual void writeCommand(uint8_t data) = 0;

    virtual void writeData(uint8_t *buff, size_t buff_size) = 0;

    virtual void InitDisplayDataTransfer() = 0;

    virtual void EndDisplayDataTransfer() = 0;

    virtual void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) = 0;

    size_t printNumber(unsigned long, uint8_t);

    size_t printFloat(double, uint8_t);

    void drawCorner(uint16_t centerX, uint16_t centerY, uint8_t radius, uint8_t quadrant, bool filled);
};

#endif
