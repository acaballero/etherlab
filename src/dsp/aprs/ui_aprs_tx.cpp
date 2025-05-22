// //
// // Created by Angel Dust on 18/05/2025.
// //

// #include "ui_aprs_tx.hpp"
// #include "ui_alphanum.hpp"

// #include "aprs.hpp"
// #include "string_format.hpp"
// #include "portapack.hpp"
// #include "baseband_api.hpp"
// #include "portapack_shared_memory.hpp"
// #include "portapack_persistent_memory.hpp"

// #include <cstring>
// #include <stdio.h>

// using namespace aprs;
// using namespace portapack;

// namespace ui {

// void APRSTXView::focus() {
//     tx_view.focus();
// }

// APRSTXView::~APRSTXView() {
//     transmitter_model.disable();
//     baseband::shutdown();
// }

// void APRSTXView::start_tx() {
//     // TODO: Clean up this API to take string_views to avoid allocations.
//     make_aprs_frame(sym_source.to_string().c_str(), num_ssid_source.value(), sym_dest.to_string().c_str(), num_ssid_dest.value(), payload);

//     // uint8_t * bb_data_ptr = shared_memory.bb_data.data;
//     // text_payload.set(to_string_hex_array(bb_data_ptr + 56, 15));

//     transmitter_model.enable();

//     baseband::set_afsk_data(AFSK_TX_SAMPLERATE / 1200, 1200, 2200, 1,
//                             10000, // APRS uses fixed 10k bandwidth
//                             8);
// }

// void APRSTXView::on_tx_progress(const uint32_t progress, const bool done) {
//     (void)progress;

//     if (done) {
//         transmitter_model.disable();
//         tx_view.set_transmitting(false);
//     }
// }

// APRSTXView::APRSTXView(NavigationView &nav) {
//     baseband::run_image(portapack::spi_flash::image_tag_afsk);

//     add_children({&labels, &sym_source, &num_ssid_source, &sym_dest, &num_ssid_dest, &text_payload, &button_set, &tx_view});

//     button_set.on_select = [this, &nav](Button &) {
//         text_prompt(nav, payload, 30, ENTER_KEYBOARD_MODE_ALPHA, [this](std::string &s) {
//             text_payload.set(s);
//         });
//     };

//     tx_view.on_edit_frequency = [this, &nav]() {
//         auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
//         new_view->on_changed = [this](rf::Frequency f) {
//             transmitter_model.set_target_frequency(f);
//         };
//     };

//     tx_view.on_start = [this]() {
//         start_tx();
//         tx_view.set_transmitting(true);
//     };

//     tx_view.on_stop = [this]() {
//         tx_view.set_transmitting(false);
//         transmitter_model.disable();
//     };
// }

// } /* namespace ui */
