//
// Created by Angel Dust on 29/03/2025.
//

#ifndef TRX_FRONTEND_SNR_WIDGET_H
#define TRX_FRONTEND_SNR_WIDGET_H

#include "Display_afb.h"
#include "ips_font.h"
#include "ui/button_widget.h"
#include "ui/label_widget.h"
#include "ui/view.h"
#include "types.h"
#include "ui/widget.h"

class SNRWidget : public View {
  public:
    SNRWidget(Rect parent_rect) : View(parent_rect) {

        lblSNR.set_font((FontDef *)&Font_7x10);
        lblSNR.set_label("S/N ");
        lblSNR.set_unit(" dB");
        lblSNR.set_padding(10);
        lblSNR.set_style(BUTTON_STYLE_FLAT);
        lblSNR.set_bg(SWAP_BYTES(RGB888_TO_RGB565(0x222233)));

        lblDbm.set_font((FontDef *)&Font_7x10);
        lblDbm.set_label("Pow ");
        lblDbm.set_unit(" dBm");
        lblDbm.set_padding(10);
        lblDbm.set_style(BUTTON_STYLE_FLAT);
        lblDbm.set_bg(SWAP_BYTES(RGB888_TO_RGB565(0x222233)));

        add_children({&lblSNR, &lblDbm});
    }

    static constexpr int height = 25;
    static constexpr int width = 105;

  protected:
    Label lblSNR{{0, 0, width, height}};
    Label lblDbm{{width + 10, 0, width, height}};

    float snr = 1e-10f;
    float dbm = 1e-10f;

    void before_paint() override;
};

#endif // TRX_FRONTEND_SNR_WIDGET_H
