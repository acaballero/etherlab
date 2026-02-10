/* -*- C++ -*- */
/********************

ILI9431 (with partial (multiple callbacks to draw a panel) DMA drawing)

***/
#ifndef MENU_ILI9431_OUT
#define MENU_ILI9431_OUT

#include "../../lib/ST77XX-STM32/st7789_fb.h"
#include "../../lib/Menu/src/menuDefs.h"
#include "dsp/dsp_buffers.h"
#include "ips_font.h"

namespace Menu {

#define RGB565(r, g, b) ((((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)))

class menuILI9431Out : public gfxOut {
  public:
    Display &gfx;
    const colorDef<uint16_t> (&colors)[nColors];

    menuILI9431Out(Display &gfx, const colorDef<uint16_t> (&c)[nColors], idx_t *t, panelsList &p, idx_t resX = 6, idx_t resY = 10, idx_t fontMargin = 3)
        : gfxOut(resX, resY, t, p, (menuOut::styles)(menuOut::redraw | menuOut::rasterDraw), fontMargin), gfx(gfx), colors(c) {
    }
    //: gfxOut(gfx.width()/resX,gfx.height()/resY,resX,resY,false),colors(c),gfx(gfx) {}

    size_t write(uint8_t ch) override {
        gfx.writeChar(ch);
        return 1;
    }

    size_t write(const uint8_t *buffer, size_t size) override {
        if (strlen((char *)buffer) > size) {
            char buff[size + 1];
            snprintf(buff, size + 1, "%.*s...", (int)(size - 3), buffer);

            gfx.write(buff, size);
        } else {
            gfx.write(buffer, size);
        }

        return size;
    }

    inline uint16_t getColor(colorDefs color = bgColor, bool selected = false, status stat = enabledStatus, bool edit = false) const {
        return memWord(&(stat == enabledStatus ? colors[color].enabled[selected + edit] : colors[color].disabled[selected]));
    }

    void setColor(colorDefs c, colorDefs b, bool selected = false, status s = enabledStatus, bool e = false) override {
        gfx.setColor(getColor(c, selected, s, e));
        gfx.setBgColor(getColor(b, selected, s, e));
    }

    void setColor(colorDefs c, bool selected = false, status s = enabledStatus, bool e = false) override {
        gfx.setColor(getColor(c, selected, s, e));
        gfx.setBgColor(getColor(bgColor, selected, s, e));
    }

    void clearLine(idx_t ln, idx_t panelNr = 0, colorDefs color = bgColor, bool selected = false, status stat = enabledStatus, bool edit = false) override {

        // No need to clear the line with this driver as the buffer is cleared at every redraw
        // const panel p = panels[panelNr];
        // gfx.fill(p.x * resX, (p.y + ln) * resY, p.maxX() * resX, (p.y + ln + 1) * resY - 1, getColor(color, selected, stat, edit));
    }

    void clear() override {
        panels.reset();
        // gfx.fillBuffer(getColor(bgColor, false, enabledStatus, false));
        setCursor(0, 0);
        setColor(fgColor);
    }

    void box(idx_t panelNr, idx_t x, idx_t y, idx_t w = 1, idx_t h = 1, colorDefs c = bgColor, bool selected = false, status stat = enabledStatus,
             bool edit = false) override {

        // TODO: To use panels, we need to call st77XX_afb::setZone(...)
        // const panel p = panels[panelNr];
        // gfx.writeRect((p.x + x) * resX, (p.y + y) * resY, w * resX, h * resY, getColor(c, selected, stat, edit));

        // The ILI9431 driver we are using (descendant of ss77XX_afb) draws pixels starting at the current zone x and y, so no need to add p.x, p.y
        gfx.writeRect((x)*resX, (y)*resY, (x + w) * resX, (y + h) * resY, getColor(c, selected, stat, edit));
    }

    void rect(idx_t panelNr, idx_t x, idx_t y, idx_t w = 1, idx_t h = 1, colorDefs c = bgColor, bool selected = false, status stat = enabledStatus,
              bool edit = false) override {
        // TODO: To use panels, we need to call st77XX_afb::setZone(...)
        // const panel p = panels[panelNr];

        gfx.fill((x)*resX, (y)*resY, (x + w) * resX, (y + h) * resY - 1, getColor(c, selected, stat, edit));
    }

    void setFont(colorDefs c) override {
        gfx.setFont(c == titleColor ? (FontDef *)&Font_Tiny8x8 : (FontDef *)&Font_7x10);
    }

    void clear(idx_t panelNr) override {

        // TODO: To use panels, we need to call st77XX_afb::setZone(...)
        // const panel p = panels[panelNr];
        // gfx.fillRect(p.x * resX, p.y * resY, p.w * resX, p.h * resY, getColor(bgColor, false, enabledStatus, false));
        //      gfx.fillBuffer(getColor(bgColor, false, enabledStatus, false));

        // panels.nodes[panelNr] = NULL;
    }

    void setCursor(idx_t x, idx_t y, idx_t panelNr = 0) override {
        // TODO: To use panels, we need to call st77XX_afb::setZone(...)
        // const panel p = panels[panelNr];
        // gfx.gotoXY((p.x + x) * resX, (p.y + y) * resY + fontMarginY);

        // The ILI9431 driver we are using (descendant of ss77XX_afb) draws pixels starting at the current zone x and y, so no need to add p.x, p.y
        gfx.gotoXY(gfx.get_padding_x() + (x)*resX, ((y)*resY) + fontMarginY);
    }

    void drawCursor(idx_t ln, bool selected, status stat, bool edit = false, idx_t panelNr = 0) override {

        // TODO: To use panels, we need to call st77XX_afb::setZone(...)
        // const panel p = panels[panelNr];

        // The ILI9431 driver we are using (descendant of ss77XX_afb) draws pixels starting at the current zone x and y, so no need to add p.x, p.y
        //  gfx.writeRect(0, (ln) * resY, maxX() * resX, (ln+1)*resY-1,
        //             getColor(cursorColor, selected, enabledStatus, false));
        uint16_t color = getColor(selectColor, selected, stat, edit);
        if (color != C565_TRANSPARENT) {
            gfx.fill(0, (ln)*resY, maxX() * resX, (ln + 1) * resY - 1, color);
        }
    }
};
}; // namespace Menu
#endif
