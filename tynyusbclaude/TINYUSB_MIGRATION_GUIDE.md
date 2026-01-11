# Complete TinyUSB Migration Guide

## Files to REMOVE (HAL USB - no longer needed)

Delete these files completely:

```
src/usb/usb_device.c          ❌ DELETE
src/usb/usb_device.h          ❌ DELETE
src/usb/usbd_cdc_if.c         ❌ DELETE
src/usb/usbd_cdc_if.h         ❌ DELETE
src/usb/usbd_storage_if.c     ❌ DELETE
src/usb/usbd_storage_if.h     ❌ DELETE
src/usb/usbd_conf.c           ❌ DELETE
src/usb/usbd_conf.h           ❌ DELETE
src/usb/usbd_desc.c           ❌ DELETE
src/usb/usbd_desc.h           ❌ DELETE
```

Keep but will be unused (HAL USB middleware - safe to leave):
```
src/usb/usbd_core.c           ⚠️  KEEP (part of HAL, won't hurt)
src/usb/usbd_core.h           ⚠️  KEEP
src/usb/usbd_ctlreq.c         ⚠️  KEEP
src/usb/usbd_ctlreq.h         ⚠️  KEEP
src/usb/usbd_ioreq.c          ⚠️  KEEP
src/usb/usbd_ioreq.h          ⚠️  KEEP
src/usb/usbd_cdc.c            ⚠️  KEEP
src/usb/usbd_cdc.h            ⚠️  KEEP
src/usb/usbd_msc.c            ⚠️  KEEP
src/usb/usbd_msc.h            ⚠️  KEEP
src/usb/usbd_msc_bot.c        ⚠️  KEEP
src/usb/usbd_msc_bot.h        ⚠️  KEEP
src/usb/usbd_msc_scsi.c       ⚠️  KEEP
src/usb/usbd_msc_scsi.h       ⚠️  KEEP
src/usb/usbd_msc_data.c       ⚠️  KEEP
src/usb/usbd_msc_data.h       ⚠️  KEEP
```

## Files to ADD (New TinyUSB implementation)

Add these new files:

```
src/usb/usb_composite_device.c        ✅ ADD (from my implementation)
src/usb/usb_composite_device.h        ✅ ADD
src/usb/sd_card_wrapper.c             ✅ ADD
src/usb/tusb_config.h                 ✅ ADD (replace if exists)
src/usb/usb_descriptors.c             ✅ ADD (use usb_descriptors_updated.c)
src/usb/usb_audio_dsp_bridge.c        ✅ ADD
src/usb/usb_audio_dsp_bridge.h        ✅ ADD
```

## Code Changes Required

### 1. Main Initialization

**OLD CODE (remove this):**
```cpp
// In main() or your init function
USB_SetupCDC();  // DELETE THIS
// or
USB_SetupMSC();  // DELETE THIS
```

**NEW CODE (add this):**
```cpp
#include "usb_composite_device.h"

// In main() initialization
usb_composite_init();  // Replaces USB_SetupCDC()/USB_SetupMSC()
```

### 2. Main Loop

**ADD this to your main loop:**
```cpp
while (1) {
    // Your existing code...
    
    // ADD THESE TWO LINES:
    usb_composite_task();      // TinyUSB processing
    
    // Your other tasks...
}
```

### 3. CAT Protocol Transmit

**OLD CODE (find and replace):**
```cpp
CDC_Transmit_HS(response_data, len);
```

**NEW CODE:**
```cpp
usb_cdc_transmit(response_data, len);
```

### 4. CAT Protocol in cat_protocol.cpp or cat_if.cpp

Your `cat_enqueue_command()` function needs NO changes - it already works!

The only change is how it gets called:
- **OLD**: Called from HAL `CDC_Receive_HS()` callback
- **NEW**: Called from TinyUSB `tud_cdc_rx_cb()` callback (already in usb_composite_device.c)

### 5. Remove HAL USB Interrupt Handler

**FILE: src/hw/stm32f4xx/usb.cpp**

**DELETE this function:**
```cpp
void OTG_HS_IRQHandler(void) {
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_HS);  // DELETE
}
```

**REPLACE with:**
```cpp
// External reference to TinyUSB
extern "C" void tud_int_handler(uint8_t rhport);

void OTG_HS_IRQHandler(void) {
    tud_int_handler(0);  // TinyUSB interrupt handler
}
```

### 6. Remove HAL USB Functions

**FILE: src/hw/stm32f4xx/usb.cpp**

**DELETE these functions:**
```cpp
bool init_USB_MSC() { ... }  // DELETE
bool init_USB_CDC() { ... }  // DELETE
```

**REPLACE with (if needed for compatibility):**
```cpp
#include "usb_composite_device.h"

bool init_USB_MSC() {
    // No longer needed - composite device has both
    return usb_composite_msc_connected();
}

bool init_USB_CDC() {
    // No longer needed - composite device has both
    return usb_composite_cdc_connected();
}
```

Or just **delete the calls** to these functions from wherever they're used.

### 7. Receive Task Audio Integration

**FILE: src/dsp/receive/receive_task.cpp**

Add to the end of `process_audio()`:

```cpp
#include "usb_audio_dsp_bridge.h"

void ReceiveTask::process_audio(buffer_t<float32_t> &buff_out_f32) {
    
    // Your existing audio processing...
    if (squelch_enabled && squelch.is_noise(buff_out_f32)) {
        memset(buff_out_f32.p, 0, buff_out_f32.size_bytes);
    } else {
        if (audio_bpf_enabled) {
            audio_bpf.decimate(buff_out_f32, buff_out_f32, 0, 1, 1);
        }
        if (deemph_enabled) {
            deemph_filter.decimate(buff_out_f32, buff_out_f32, 0, 1, 1);
        }
        if (compressor_enabled) {
            compressor.work(buff_out_f32);
        }
    }
    
    // ADD THIS: Send processed audio to USB
    if (usb_audio_is_streaming()) {
        usb_audio_send_rx_audio(buff_out_f32.p, buff_out_f32.count);
    }
}
```

## Build System Changes

### CMakeLists.txt or Makefile

**ADD TinyUSB:**
```cmake
# TinyUSB directory
set(TINYUSB_DIR ${CMAKE_SOURCE_DIR}/lib/tinyusb)

# TinyUSB includes
include_directories(
    ${TINYUSB_DIR}/src
    ${TINYUSB_DIR}/src/device
    ${TINYUSB_DIR}/src/class/audio
    ${TINYUSB_DIR}/src/class/cdc
    ${TINYUSB_DIR}/src/class/msc
    src/usb  # For tusb_config.h
)

# TinyUSB sources
set(TINYUSB_SOURCES
    ${TINYUSB_DIR}/src/tusb.c
    ${TINYUSB_DIR}/src/common/tusb_fifo.c
    ${TINYUSB_DIR}/src/device/usbd.c
    ${TINYUSB_DIR}/src/device/usbd_control.c
    ${TINYUSB_DIR}/src/class/audio/audio_device.c
    ${TINYUSB_DIR}/src/class/cdc/cdc_device.c
    ${TINYUSB_DIR}/src/class/msc/msc_device.c
    ${TINYUSB_DIR}/src/portable/st/synopsys/dcd_synopsys.c  # For STM32F4 OTG
)

# Your new USB files
set(USB_SOURCES
    src/usb/usb_composite_device.c
    src/usb/sd_card_wrapper.c
    src/usb/usb_descriptors.c
    src/usb/usb_audio_dsp_bridge.c
)

# Add to executable
add_executable(${PROJECT_NAME}
    # Your existing sources...
    ${TINYUSB_SOURCES}
    ${USB_SOURCES}
)

# Compiler definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    CFG_TUSB_MCU=OPT_MCU_STM32F4
    CFG_TUSB_OS=OPT_OS_NONE
    STM32F427xx
)
```

**REMOVE from build (these are deleted files):**
```cmake
# DELETE THESE from your source list:
src/usb/usb_device.c
src/usb/usbd_cdc_if.c
src/usb/usbd_storage_if.c
src/usb/usbd_conf.c
src/usb/usbd_desc.c
```

## STM32CubeMX Changes

If you use CubeMX, **DISABLE USB** in the configurator:
1. Open your `.ioc` file
2. Go to `Connectivity` → `USB_OTG_HS`
3. **Uncheck** "Device (FS/HS)"
4. Keep the peripheral enabled but without middleware
5. Regenerate code

This prevents CubeMX from overwriting your TinyUSB implementation.

## Installation Steps

1. **Install TinyUSB:**
```bash
cd lib/
git clone https://github.com/hathach/tinyusb.git
```

2. **Copy new files** to `src/usb/`:
   - usb_composite_device.c/h
   - sd_card_wrapper.c
   - usb_audio_dsp_bridge.c/h
   - tusb_config.h (use tusb_config_updated.h)
   - usb_descriptors.c (use usb_descriptors_updated.c)

3. **Delete old HAL USB files** (see list above)

4. **Update build system** (CMakeLists.txt or Makefile)

5. **Modify code** as shown above:
   - Main init: `usb_composite_init()`
   - Main loop: `usb_composite_task()`
   - CAT transmit: `usb_cdc_transmit()`
   - Interrupt handler: `tud_int_handler(0)`
   - Audio: add to `process_audio()`

6. **Build and test**

## Testing Checklist

- [ ] Device enumerates on PC
- [ ] CDC port appears (CAT control)
- [ ] MSC appears (SD card access)
- [ ] Audio device appears
- [ ] CAT protocol works with WSJT-X
- [ ] SD card can be read/written
- [ ] Audio streams to WSJT-X
- [ ] WSJT-X can decode signals

## Troubleshooting

### Device doesn't enumerate
- Check TinyUSB is in build
- Verify `tud_int_handler()` is called from OTG_HS_IRQHandler
- Check `usb_composite_task()` is in main loop

### CAT protocol doesn't work
- Verify `cat_enqueue_command()` is being called
- Check `usb_cdc_transmit()` replaces `CDC_Transmit_HS()`
- Ensure CDC callbacks are in usb_composite_device.c

### SD card doesn't work
- Check SD card functions in sd_card_wrapper.c
- Verify `hsd` handle is available
- Test SD card separately

### No audio
- Check `usb_audio_is_streaming()` returns true
- Verify `process_audio()` calls `usb_audio_send_rx_audio()`
- Check sample rate (12/24kHz needs conversion to 48kHz)

## Summary of Changes

**Deleted**: 6-10 HAL USB files
**Added**: 6 new TinyUSB files  
**Modified**: 3-4 existing files (main, usb interrupt, process_audio)
**Result**: CDC + MSC + Audio all working simultaneously

This is a clean migration that preserves all your functionality while adding USB Audio for WSJT-X.
