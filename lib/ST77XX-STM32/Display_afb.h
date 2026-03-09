#ifndef __ST77XX_AFB_H
#define __ST77XX_AFB_H

#include "stm32f4xx.h"
#include "string.h"
#include "ui/ui_types.h"
#include <stdint.h>

#define DEBUG_LCD 1

#define DISPLAY_SLICE_HEIGHT 16

#ifndef DISPLAY_X_PIXELS
#if USING_HORIZONAL == 0 || USING_HORIZONAL == 1
#define DISPLAY_PADDING 8
#define DISPLAY_TOTAL_WIDTH 480
#define DISPLAY_X_PIXELS (DISPLAY_TOTAL_WIDTH - (DISPLAY_PADDING << 1))
#define DISPLAY_Y_PIXELS (320 - (DISPLAY_PADDING << 1))
#else
#define DISPLAY_X_PIXELS 240
#define DISPLAY_Y_PIXELS 240
#endif
#endif

#include "Painter.hpp"
#include "ips_font.h"
#include <stm32f4xx.h>

#define RGB888_TO_RGB565(rgb) ((((rgb >> 19) & 0x1f) << 11) | (((rgb >> 10) & 0x3f) << 5) | (((rgb >> 3) & 0x1f)))
#define RGB565_TO_BGR565(rgb) (((rgb)&0x07E0) | (((rgb)&0xF800) >> 11) | (((rgb)&0x001F) << 11))

#define SWAP_BYTES(w) (uint16_t)(w >> 8 | w << 8)

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
#define C565_DARKEST 0x0700
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
#define C565_YELLOW_LIGHT 0xF5FF
#define C565_YELLOW_DARK 0x4ab4
#define C565_YELLOW 0xE0FF
#define C565_ORANGE 0xA0FD
#define C565_GREENYELLOW 0xE0B7
#define C565_PINK 0x9FFC
#define C565_BROWN 0x609A
#define C565_GOLD 0xA0FE
#define C565_SILVER 0x18C6
#define C565_SKYBLUE 0x4711
#define C565_VIOLET 0x6828
#define C565_TRANSPARENT 0xFFFE

#define C565_TEXT_FG C565_WHITE
#define C565_FIELD_FG C565_CYAN
#define C565_BUTTON_TEXT_FG C565_BLACK
#define C565_TEXT_FG_DISABLED C565_GREY_LIGHT
#define C565_TEXT_FG_FOCUS C565_YELLOW
#define C565_TEXT_BG C565_TRANSPARENT
#define C565_BG C565_GREY_LIGHT
#define C565_BG_DISABLED C565_GREY_DARK
#define C565_BG_FOCUS C565_WHITE
#define C565_BG_ENABLED C565_WHITE
#define C565_UNITS_FG C565_GREY_LIGHT

#define END_DMA_TRANSFER                                                                                                                                       \
    {                                                                                                                                                          \
        while (HAL_SPI_GetState(spi_port) != HAL_SPI_STATE_READY) {                                                                                            \
            ;                                                                                                                                                  \
        }                                                                                                                                                      \
        EndDisplayDataTransfer();                                                                                                                              \
    }

using Color = uint16_t;

extern Color palette16[16];

struct DisplayPoint {
    uint32_t x, y;
};
struct DisplaySize {
    uint32_t w, h;
};

class Display {

  public:
    Display(SPI_HandleTypeDef *);

    virtual int16_t begin() = 0;

    virtual int16_t stop() = 0;

    virtual int16_t backlight(bool b) = 0;

    virtual void select() = 0;

    virtual void unselect() = 0;

    virtual void reset() = 0;

    virtual void scroll(int step){};

    bool draw_area(Area *area, Painter *painter);

    bool draw_area(Area *, Painter *, bool pad_display);

    uint8_t renderString(int8_t x, int8_t y, uint16_t length);

    uint16_t *getBuffer();

    void setPixel(int16_t x, int16_t y, uint16_t color);

    void writeVertLine(int16_t x, int16_t y1, int16_t y2, uint16_t color);

    void writeLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

    void writeLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

    void writeLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color, uint8_t width);

    void writeRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

    void writeRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

    void writeRect(DisplayPoint p, DisplaySize s, Color c);

    void drawCircle(int16_t x0, int16_t y0, uint8_t r, uint16_t color);

    void invertColors(uint8_t invert);

    void test(void);

    void setFont(const FontDef *);

    const FontDef *getFont(void);

    void setColor(uint16_t c);

    void set_padding(uint16_t x, uint16_t y);

    uint16_t get_padding_x();

    uint16_t get_padding_y();

    void setBgColor(uint16_t c);

    uint16_t getColor();

    void clear(Color color = C565_BLACK);

    void gotoXY(int16_t x, int16_t y);

    uint16_t getY();

    void gotoCharXY(int16_t x, int16_t y);

    void fill(DisplayPoint p, DisplaySize s, Color c);

    void fill(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color c);

    void fillBuffer(Color color);

    void writeChar(int16_t x, int16_t y, char ch, const FontDef *font, uint16_t color, uint16_t bgcolor);

    void writeChar(char ch);

    void writeString(int16_t x, int16_t y, const char *str, const FontDef *font, uint16_t color, uint16_t bgcolor);

    void drawRoundedRectangle(int16_t x0, int16_t y0, uint16_t width, uint16_t height, uint16_t radius, bool filled, bool top_left, bool top_right,
                              bool bottom_left, bool bottom_right);

    void drawRoundedRectangle(int16_t x0, int16_t y0, uint16_t width, uint16_t height, uint16_t radius, bool filled);

    std::string fit_text(const std::string &text, int max_width = -1, int max_height = -1);

    uint32_t get_punctuation_width(const FontDef *font = nullptr);

    bool is_punctuation(char c);

    size_t write(uint8_t uint8_t);

    size_t write(const uint8_t *buffer, size_t size);

    size_t write(const char *str) {
        if (str == NULL) {
            return 0;
        }
        return write((const uint8_t *)str, strlen(str));
    }

    size_t write(const char *buffer, size_t size) {
        return write((const uint8_t *)buffer, size);
    }

    // These handle ambiguity for write(0) case, because (0) can be a pointer or
    // an integer
    inline size_t write(short t) {
        return write((uint8_t)t);
    }

    inline size_t write(unsigned short t) {
        return write((uint8_t)t);
    }

    inline size_t write(int t) {
        return write((uint8_t)t);
    }

    inline size_t write(unsigned int t) {
        return write((uint8_t)t);
    }

    inline size_t write(long t) {
        return write((uint8_t)t);
    }

    inline size_t write(unsigned long t) {
        return write((uint8_t)t);
    }

    // Enable write(char) to fall through to write(uint8_t)
    inline size_t write(char c) {
        return write((uint8_t)c);
    }

    inline size_t write(int8_t c) {
        return write((uint8_t)c);
    }

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
    void setOffset(Box r);

    bool hasOffset();

    Box get_offset();

    void clear_offset();

    void set_enabled(bool);

    bool get_wrap_text() const;

    bool get_use_dma() {
        return use_dma;
    }

    void set_use_dma(bool v) {
        use_dma = v;
    }

    void set_wrap_text(bool wrap_text);

    void set_trim_enabled(bool b);

    bool get_trim_enabled();

    void set_transparency(uint8_t v);

    Size get_text_size(const std::string &str);
    Size get_text_size(const char *r, const FontDef *f = nullptr);

    uint8_t get_transparency();

    bool can_interrupt() {
        return !busy || curr_buffer == b565_buffer;
        // if (!ok) {
        //     interrupted = true;
        // }
        // return true;
    }

    uint16_t current_line = 0;
    uint16_t chunk_height = 0;
    uint16_t current_last_line = 0;
    volatile bool DMAHalfTransferCompleted = false;
    volatile bool busy = false;
    //    volatile bool interrupted = false;
    bool use_dma = true;
    // Display buffer area
    Area *curr_area = 0;

  protected:
    static void fill_callback(Display *, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

    SPI_HandleTypeDef *spi_port;

  private:
    bool enabled = true;
    bool wrap_text = true;
    bool trim_enabled = true;
    int16_t ox = 0;  // offset x
    int16_t oy = 0;  // offset y
    uint16_t ow = 0; // offset width
    uint16_t oh = 0; // offset height
    uint16_t px = 0;
    uint16_t py = 0;
    uint16_t padding_x = 0, padding_y = 0;
    const FontDef *font = (FontDef *)&Font_11x18;
    uint16_t color = C565_WHITE;
    uint16_t bgColor = C565_BLACK;
    uint8_t transparency = 0;
    uint8_t verticalSpacing = 2;
    uint16_t *curr_buffer = 0;

    uint32_t dma_transfer_length;

    /* RGB565 buffer for transferring pixels to the display using DMA */
    static constexpr uint16_t b565_buffer_size = DISPLAY_TOTAL_WIDTH * DISPLAY_SLICE_HEIGHT;

    __attribute__((aligned(2))) uint16_t b565_buffer[b565_buffer_size];

    // Clipping rectangle
    Box clip_box;

    virtual HAL_StatusTypeDef writeCommand(uint8_t data) = 0;

    virtual HAL_StatusTypeDef writeData(uint8_t *buff, size_t buff_size) = 0;

    virtual HAL_StatusTypeDef InitDisplayDataTransfer() = 0;

    virtual HAL_StatusTypeDef EndDisplayDataTransfer() = 0;

    virtual HAL_StatusTypeDef setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) = 0;

    size_t print_number(unsigned long, uint8_t);

    size_t print_float(double, uint8_t);

    uint32_t calculate_buffer_checksum();

    void draw_corner(int16_t centerX, int16_t centerY, uint8_t radius, uint8_t quadrant, bool filled);

#if 0 // LCD_ENABLE_BUFFER_SKIP

    struct Slice {
        int16_t area_x, area_y;
        uint16_t slice_y;
        uint32_t checksum{0}; // 0 = invalid
    };

    static const uint16_t MAX_SLICES = 100;
    Slice slice_checksums[MAX_SLICES];
    uint8_t next_slice_index = 0;

    int find_zone(int16_t area_x, int16_t area_y, uint16_t slice_y) {
        for (int i = 0; i < MAX_SLICES; i++) {
            Slice *slice = &slice_checksums[i];
            if (slice->checksum != 0 && slice->area_x == area_x && slice->area_y == area_y && slice->slice_y == slice_y) {
                return i;
            }
        }
        return -1;
    }
#endif
    void check_dma_transfer_length();

    void spi_transfer(uint16_t size);
};

#endif
