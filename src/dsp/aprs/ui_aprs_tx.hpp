//
// Created by Angel Dust on 18/05/2025.
//

namespace dsp {

class APRSTXView : public View {
  public:
    APRSTXView(NavigationView &nav);
    ~APRSTXView();

    void focus() override;

    std::string title() const override {
        return "APRS TX";
    };

  private:
    TxRadioState radio_state_{
        144390000 /* frequency */, 1750000 /* bandwidth */, AFSK_TX_SAMPLERATE /* sampling rate */
    };
    app_settings::SettingsManager settings_{"tx_aprs", app_settings::Mode::TX};

    std::string payload{""};

    void start_tx();
    void generate_frame();
    void generate_frame_pos();
    void on_tx_progress(const uint32_t progress, const bool done);

    Labels labels{
        {{0 * 8, 1 * 16}, "Source:       SSID:", Theme::getInstance()->fg_light->foreground}, // 6 alphanum + SSID
        {{0 * 8, 2 * 16}, " Dest.:       SSID:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 * 16}, "Info field:", Theme::getInstance()->fg_light->foreground},
    };

    SymField sym_source{{7 * 8, 1 * 16}, 6, SymField::Type::Alpha};

    NumberField num_ssid_source{{19 * 8, 1 * 16}, 2, {0, 15}, 1, ' '};

    SymField sym_dest{{7 * 8, 2 * 16}, 6, SymField::Type::Alpha};

    NumberField num_ssid_dest{{19 * 8, 2 * 16}, 2, {0, 15}, 1, ' '};

    Text text_payload{{0 * 8, 5 * 16, 30 * 8, 16}, "-"};
    Button button_set{{0 * 8, 6 * 16, 80, 32}, "Set"};

    TransmitterView tx_view{
        16 * 16, 5000,
        0 // disable setting bandwith, since APRS used fixed 10k bandwidth
    };

    MessageHandlerRegistration message_handler_tx_progress{Message::ID::TXProgress, [this](const Message *const p) {
                                                               const auto message = *reinterpret_cast<const TXProgressMessage *>(p);
                                                               this->on_tx_progress(message.progress, message.done);
                                                           }};
};

} // namespace dsp
