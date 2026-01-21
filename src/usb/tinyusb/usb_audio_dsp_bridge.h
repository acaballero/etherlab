#ifndef USB_AUDIO_DSP_BRIDGE_H
#define USB_AUDIO_DSP_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

/**
 * USB audio DSP bridge
 *
 * RX mode (devide -> PC):
 *   ADC (I/Q IF) -> decimators -> demodulator -> process_audio() -> DAC (speaker)
 *                                                                  -> USB audio (PC)
 *
 * TX mode (PC -> device):
 *   USB audio (PC) -> modulator -> interpolators -> DAC (I/Q IF)
 */

// Audio format for most host apps
//#define USB_AUDIO_CHANNELS 1 // Mono audio
//#define USB_AUDIO_BIT_DEPTH 16

// Buffer for USB audio samples
#define USB_AUDIO_BUFFER_SAMPLES 196 // 4ms at 48kHz
//#define USB_AUDIO_BUFFER_SIZE (USB_AUDIO_BUFFER_SAMPLES * sizeof(int16_t))

// Initialize USB audio bridge
void usb_audio_dsp_bridge_init(void);

// Start/stop USB audio streaming
void usb_audio_dsp_bridge_start(uint8_t itf_ix);
void usb_audio_dsp_bridge_stop(uint8_t itf_ix);

// Called from process_audio() to send demodulated audio to USB
// This should be called AFTER  process_audio() has done filtering/compression
// void usb_audio_send_rx_audio(const int16_t *audio_samples, uint16_t count);

// Called from  TX modulator to get audio from USB
// Returns number of samples written to audio_samples
// uint16_t usb_audio_get_tx_audio(float32_t *audio_samples, uint16_t max_count);

// Retruns true if USB audio is active for an interface
bool usb_audio_is_streaming(uint8_t itf_ix);

// Process USB tasks
// void usb_audio_process(void);

/*
 * Expects 12-bit resolution, single channel samples
 * sample_rate must be lower thant USB_AUDIO_SAMPLE_RATE and the buffer is interpolated if that case
 */
void usb_audio_send(int16_t *buffer, uint32_t count, uint16_t sample_rate);

/*
 * Returns 12-bit resolution, single channel samples at the specified sample_rate (decimates/interpolates if necessary)
 * @return the number of samples actually received
 */
uint16_t usb_audio_receive(int16_t *buffer, uint32_t count, uint16_t sample_rate);

#ifdef __cplusplus
}
#endif

#endif // USB_AUDIO_DSP_BRIDGE_H
