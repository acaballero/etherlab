# Heap allocation size report (explicit allocations)

ELF: `/home/ahcr/dev/trx/.pio/build/genericSTM32F427VGT/firmware.elf`

This report covers explicit heap allocation sites found in `/src/**` and `/lib/Menu/src/**`, and uses `arm-none-eabi-gdb` on the ELF DWARF to query `sizeof(T)` for each discovered type.

Resolved sizes: 60
Unresolved sizes: 0

## Sizes

| Type (as found) | sizeof (bytes) | Resolved as (gdb) | Alloc sites (first few) |
|---|---:|---|---|
| `AFSKTXTask` | 464 | `dsp::AFSKTXTask` | /home/ahcr/dev/trx/src/dsp/aprs/aprs_ui.cpp:328 (make_unique) |
| `APRSTask` | 1840 | `dsp::APRSTask` | /home/ahcr/dev/trx/src/dsp/aprs/aprs_ui.cpp:166 (make_unique) |
| `BeaconSettingsView` | 2648 | `dsp_ui::BeaconSettingsView` | /home/ahcr/dev/trx/src/dsp/aprs/aprs_ui.cpp:135 (make_unique) |
| `CaptureTask` | 376 |  | /home/ahcr/dev/trx/src/dsp/dsp.cpp:100 (make_unique) |
| `Color` | 2 |  | /home/ahcr/dev/trx/src/ui/map_view.cpp:83 (new[]) |
| `DspCaptureProcessor` | 2264 |  | /home/ahcr/dev/trx/src/dsp/capture/capture_task.h:43 (make_unique) |
| `DspOOKProcessor` | 184 |  | /home/ahcr/dev/trx/src/dsp/ook/ook_task.h:34 (make_unique) |
| `DspReceiveProcessor` | 88 |  | /home/ahcr/dev/trx/src/dsp/receive/receive_task_base.h:65 (make_unique) |
| `DspReplayProcessor` | 88 |  | /home/ahcr/dev/trx/src/dsp/afsk/afsk_tx_task.h:37 (make_unique); /home/ahcr/dev/trx/src/dsp/replay/replay_task.h:50 (make_unique) |
| `DspSignalGeneratorProcessor` | 208 |  | /home/ahcr/dev/trx/src/dsp/signal_generator/signal_generator_task.h:32 (make_unique) |
| `DspTransmitProcessor` | 88 |  | /home/ahcr/dev/trx/src/dsp/transmit/transmit_task.h:57 (make_unique) |
| `FSO` | 1432 |  | /home/ahcr/dev/trx/src/ui/sd_filepicker_menu.h:74 (make_unique) |
| `File` | 36 |  | /home/ahcr/dev/trx/src/io/file_factory.cpp:16 (new) |
| `Impl` | 588 | `io::directory_iterator::Impl` | /home/ahcr/dev/trx/src/io/fatfs_file.cpp:501 (make_shared) |
| `LockView` | 752 |  | /home/ahcr/dev/trx/src/ui/menu.cpp:273 (new) |
| `LogFile` | 584 |  | /home/ahcr/dev/trx/src/dsp/radiosonde/radiosonde_ui.cpp:22 (make_unique); /home/ahcr/dev/trx/src/dsp/aprs/aprs_ui.cpp:49 (make_unique) |
| `Menu::menuNodeShadow` | 24 |  | (no direct site recorded; may be internal/template) |
| `Menu::menuValue<FFT_VIEW_MODE>` | 12 |  | /home/ahcr/dev/trx/src/dsp/fft/fft_ui.cpp:79 (new); /home/ahcr/dev/trx/src/dsp/fft/fft_ui.cpp:80 (new) |
| `Menu::menuValue<FFT_WINDOW_TYPES>` | 12 |  | /home/ahcr/dev/trx/src/dsp/fft/fft_ui.cpp:70 (new); /home/ahcr/dev/trx/src/dsp/fft/fft_ui.cpp:71 (new) |
| `Menu::menuValue<FileType>` | 12 |  | /home/ahcr/dev/trx/src/dsp/capture/dsp_capture_ui.cpp:217 (new); /home/ahcr/dev/trx/src/dsp/capture/dsp_capture_ui.cpp:218 (new) |
| `Menu::menuValue<IF_GAIN>` | 12 |  | /home/ahcr/dev/trx/src/hw/board/board_v2_ui.cpp:18 (new); /home/ahcr/dev/trx/src/hw/board/board_v2_ui.cpp:19 (new); /home/ahcr/dev/trx/src/hw/board/board_v2_ui.cpp:20 (new); +7 more |
| `Menu::menuValueShadow<FFT_VIEW_MODE>` | 16 |  | (no direct site recorded; may be internal/template) |
| `Menu::menuValueShadow<FFT_WINDOW_TYPES>` | 16 |  | (no direct site recorded; may be internal/template) |
| `Menu::menuValueShadow<FileType>` | 16 |  | (no direct site recorded; may be internal/template) |
| `Menu::menuValueShadow<IF_GAIN>` | 16 |  | (no direct site recorded; may be internal/template) |
| `Menu::menuVariantShadow<FileType>` | 28 |  | (no direct site recorded; may be internal/template) |
| `Menu::menuVariantShadow<IF_GAIN>` | 28 |  | (no direct site recorded; may be internal/template) |
| `Menu::menuVariantShadow<uint8_t>` | 28 | `Menu::menuVariantShadow<unsigned char>` | (no direct site recorded; may be internal/template) |
| `Menu::promptShadow` | 16 |  | (no direct site recorded; may be internal/template) |
| `Menu::select<FileType>` | 12 |  | /home/ahcr/dev/trx/src/dsp/capture/dsp_capture_ui.cpp:221 (new) |
| `Menu::select<IF_GAIN>` | 12 |  | /home/ahcr/dev/trx/src/hw/board/board_v2_ui.cpp:31 (new); /home/ahcr/dev/trx/src/hw/board/board_v2_ui.cpp:34 (new) |
| `Menu::select<uint8_t>` | 12 | `Menu::select<unsigned char>` | /home/ahcr/dev/trx/src/dsp/fft/fft_ui.cpp:77 (new); /home/ahcr/dev/trx/src/dsp/fft/fft_ui.cpp:82 (new) |
| `Menu::textFieldShadow` | 28 |  | (no direct site recorded; may be internal/template) |
| `ModalView` | 1536 |  | /home/ahcr/dev/trx/src/ui/sd_filepicker_menu.h:148 (make_unique); /home/ahcr/dev/trx/src/ui/modal_view.h:24 (make_unique) |
| `NumberEditView` | 2872 |  | /home/ahcr/dev/trx/src/ui/menu_prompts.hpp:133 (make_unique) |
| `OOKTask` | 376 |  | /home/ahcr/dev/trx/src/dsp/ook/dsp_ook_ui.cpp:158 (make_unique) |
| `ReceiveTask` | 896 |  | /home/ahcr/dev/trx/src/dsp/dsp.cpp:109 (make_unique) |
| `ReplayTask` | 400 |  | /home/ahcr/dev/trx/src/dsp/dsp.cpp:103 (make_unique) |
| `SignalGeneratorTask` | 376 |  | /home/ahcr/dev/trx/src/dsp/dsp.cpp:106 (make_unique) |
| `TransmitTask` | 528 |  | /home/ahcr/dev/trx/src/dsp/dsp.cpp:112 (make_unique) |
| `WaveFile` | 136 |  | /home/ahcr/dev/trx/src/io/file_factory.cpp:12 (new) |
| `dsp::RadiosondeTask` | 1544 |  | /home/ahcr/dev/trx/src/dsp/radiosonde/radiosonde_ui.cpp:111 (make_unique) |
| `dsp::am_demodulator` | 4 |  | /home/ahcr/dev/trx/src/dsp/receive/receive_task_base.cpp:295 (make_unique) |
| `dsp::am_modulator` | 8 |  | /home/ahcr/dev/trx/src/dsp/transmit/transmit_task.cpp:196 (make_unique) |
| `dsp::fm_demodulator` | 28 |  | /home/ahcr/dev/trx/src/dsp/receive/receive_task_base.cpp:301 (make_unique); /home/ahcr/dev/trx/src/dsp/receive/receive_task_base.cpp:305 (make_unique) |
| `dsp::fm_modulator` | 20 |  | /home/ahcr/dev/trx/src/dsp/transmit/transmit_task.cpp:212 (make_unique) |
| `dsp::ssb_demodulator` | 4 |  | /home/ahcr/dev/trx/src/dsp/receive/receive_task_base.cpp:291 (make_unique); /home/ahcr/dev/trx/src/dsp/receive/receive_task_base.cpp:299 (make_unique); /home/ahcr/dev/trx/src/dsp/receive/receive_task_base.cpp:309 (make_unique) |
| `dsp::ssb_modulator` | 408 |  | /home/ahcr/dev/trx/src/dsp/transmit/transmit_task.cpp:200 (make_unique); /home/ahcr/dev/trx/src/dsp/transmit/transmit_task.cpp:204 (make_unique); /home/ahcr/dev/trx/src/dsp/transmit/transmit_task.cpp:208 (make_unique) |
| `dsp_ui::APRSView` | 2960 |  | /home/ahcr/dev/trx/src/dsp/dsp_ui.cpp:87 (make_unique) |
| `dsp_ui::RadiosondeView` | 5656 |  | /home/ahcr/dev/trx/src/dsp/dsp_ui.cpp:93 (make_unique) |
| `fieldBaseShadow` | 20 | `Menu::fieldBaseShadow` | /home/ahcr/dev/trx/src/ui/menu_frequency.h:21 (new) |
| `menuNodeShadow` | 24 | `Menu::menuNodeShadow` | /home/ahcr/dev/trx/lib/Menu/src/items.h:402 (new) |
| `os::periodic_task` | 88 |  | /home/ahcr/dev/trx/src/dsp/aprs/aprs_ui.cpp:141 (new) |
| `periodic_task` | 88 | `os::periodic_task` | /home/ahcr/dev/trx/src/os/task_manager.cpp:69 (new) |
| `promptShadow` | 16 | `Menu::promptShadow` | /home/ahcr/dev/trx/lib/Menu/src/items.h:66 (new) |
| `radiosonde::Packet` | 372 |  | /home/ahcr/dev/trx/src/dsp/radiosonde/radiosonde_ui.cpp:148 (make_unique) |
| `samples_t` | 8 | `dsp::matched_filter::MatchedFilter::sample_t` | /home/ahcr/dev/trx/src/dsp/blocks/matched_filter.cpp:10 (make_unique) |
| `taps_t` | 8 | `dsp::matched_filter::MatchedFilter::tap_t` | /home/ahcr/dev/trx/src/dsp/blocks/matched_filter.cpp:11 (make_unique) |
| `textFieldShadow` | 28 | `Menu::textFieldShadow` | /home/ahcr/dev/trx/lib/Menu/src/items.h:226 (new) |
| `ui::MapView` | 5952 |  | /home/ahcr/dev/trx/src/dsp/radiosonde/radiosonde_ui.cpp:64 (make_unique); /home/ahcr/dev/trx/src/dsp/aprs/aprs_ui.cpp:286 (make_unique) |

## Notes

- `std::vector`, `std::string`, and `std::function` internal allocations are not enumerated here; they depend on runtime sizes/capacities.
- `new T[n]` sites are reported as `sizeof(T)` only; runtime `n` is not known statically.
