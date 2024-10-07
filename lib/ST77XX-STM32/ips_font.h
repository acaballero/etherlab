#ifndef __IPS_FONT_H
#define __IPS_FONT_H

#include "stdint.h"

#define ICON_BATT_LOW 32
#define ICON_BATT_MID 33
#define ICON_BATT_FULL 34
#define ICON_BATT_CHARGING 35
#define ICON_HERTZ 36
#define ICON_ARROW_BACK 39
#define ICON_ARROW_FORWARD 40
#define ICON_SD_CARD 41
#define ICON_USB 42
#define ICON_DIGITAL 43
#define ICON_ANALOG 44
#define ICON_SOUND_ON 45
#define ICON_SOUND_OFF 46

enum FontEncoding {
    ROWS,
    COLUMNS
};

struct FontDef {
    uint8_t encoding;
    uint8_t size;
    uint8_t width;
    uint8_t height;
    uint8_t trim_punct_start;
    uint8_t trim_punct_end;
};

typedef struct {
    struct FontDef super;
    const uint16_t *data;
} FontDef16;

typedef struct {
    struct FontDef super;
    const uint8_t *data;
} FontDef8;

extern const FontDef16 Font_Micro4x6;
extern const FontDef16 Font_Tiny8x8;
extern const FontDef16 Font_7x10;
extern const FontDef16 Font_11x18;
extern const FontDef8 Font_Fixed5x7;
extern const FontDef16 Font_Icons9x8;
//extern const FontDef Font_Sinclair8x8;
//extern const FontDef Font_16x26;

extern uint8_t ST7789_32[];
extern uint8_t ST7789_16[];

#endif