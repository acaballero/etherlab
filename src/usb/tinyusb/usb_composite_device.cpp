/**
 * usb_composite_device.c
 *
 * Complete TinyUSB composite device implementation
 * Replaces all HAL USB code with TinyUSB for CDC+MSC+Audio
 */

#include "../../../lib/tinyusb/src/tusb.h"
#include "device/usbd.h"
#include "hw/stm32f4xx/usb.h"
#include "tinyusb/tusb_config.h"
#include "usb_composite_device.h"
#include "usb_audio_dsp_bridge.h"
#include "io/cat_if.h"
#include "hw/stm32_hal.h"
#include <string.h>

// SD card functions (you'll need to provide these)
extern int sd_card_read_blocks(uint32_t lba, uint8_t *buffer, uint32_t block_count);
extern int sd_card_write_blocks(uint32_t lba, const uint8_t *buffer, uint32_t block_count);
extern uint32_t sd_card_get_block_count(void);
extern bool sd_card_is_ready(void);

// USB device state
static bool cdc_connected = false;
static bool msc_connected = false;

//--------------------------------------------------------------------+
// INITIALIZATION
//--------------------------------------------------------------------+

void usb_composite_init(void) {

    // Initialize USB hardware
    MX_USB_OTG_HS_Init();
    // Initialize TinyUSB
    tusb_rhport_init(BOARD_TUD_RHPORT, NULL);

    // Initialize audio bridge
    usb_audio_dsp_bridge_init();
    usb_audio_dsp_bridge_start();

    cdc_connected = false;
    msc_connected = false;
}

void usb_composite_task(void) {
    // TinyUSB device task - must be called frequently
    tud_task_ext(1, false);

    // Audio processing
    usb_audio_process();
}

bool usb_composite_cdc_connected(void) {
    return cdc_connected;
}

bool usb_composite_msc_connected(void) {
    return msc_connected;
}

//--------------------------------------------------------------------+
// CDC CALLBACKS - Your CAT Protocol
//--------------------------------------------------------------------+

// Invoked when CDC interface receives data from host
void tud_cdc_rx_cb(uint8_t itf) {
    (void)itf; // Interface index (we only have one CDC)

    uint8_t buf[64];
    uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));

    if (count > 0) {
        // Forward to your existing CAT protocol handler
        cat_enqueue_command((char *)buf, count);
    }
}

// Invoked when a TX is complete and more data can be sent
void tud_cdc_tx_complete_cb(uint8_t itf) {
    (void)itf;
    // Can be used if you have a TX queue
}

// Invoked when line state changes (DTR/RTS)
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void)itf;
    (void)rts;

    // DTR = Data Terminal Ready (PC has opened the port)
    cdc_connected = dtr;
}

// Invoked when line coding is changed (baud rate, etc)
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *p_line_coding) {
    (void)itf;
    (void)p_line_coding;
    // CAT protocol typically doesn't care about baud rate
}

// Function to send data via CDC (replaces CDC_Transmit_HS)
bool usb_cdc_transmit(const uint8_t *data, uint16_t len) {
    if (!cdc_connected || !tud_cdc_connected()) {
        return false;
    }

    uint32_t sent = 0;
    while (sent < len) {
        uint32_t available = tud_cdc_write_available();
        if (available == 0) {
            tud_cdc_write_flush();
            continue;
        }

        uint32_t to_send = (len - sent) < available ? (len - sent) : available;
        uint32_t written = tud_cdc_write(data + sent, to_send);
        sent += written;
    }

    tud_cdc_write_flush();
    return true;
}

//--------------------------------------------------------------------+
// MSC CALLBACKS - Your SD Card Access
//--------------------------------------------------------------------+

// Invoked when received SCSI_CMD_INQUIRY
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {

    if (!usb_get_msc_enabled()) { // Does nothing if MSC is explicitly disabled
        return;
    }

    (void)lun;

    const char vid[] = "Angel Dust";
    const char pid[] = "Etherlab EL24 SDR Transceiver";
    const char rev[] = "1.0";

    memcpy(vendor_id, vid, strlen(vid));
    memcpy(product_id, pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    (void)lun;

    msc_connected = true;
    return sd_card_is_ready();
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY
void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    (void)lun;

    *block_count = sd_card_get_block_count();
    *block_size = 512;
}

// Invoked when received Start Stop Unit command
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    (void)lun;
    (void)power_condition;
    (void)start;
    (void)load_eject;

    return true;
}

// Callback invoked when received READ10 command
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    (void)lun;
    (void)offset; // TinyUSB handles offset internally

    // Calculate number of blocks to read
    uint32_t block_count = bufsize / 512;

    // Read from SD card using your existing function
    if (sd_card_read_blocks(lba, (uint8_t *)buffer, block_count) == 0) {
        return bufsize;
    }

    return -1; // Error
}

// Callback invoked when received WRITE10 command
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    (void)lun;
    (void)offset;

    uint32_t block_count = bufsize / 512;

    if (sd_card_write_blocks(lba, buffer, block_count) == 0) {
        return bufsize;
    }

    return -1;
}

// Callback invoked when WRITE10 command is completed
void tud_msc_write10_complete_cb(uint8_t lun) {
    (void)lun;
    // Optional: flush SD card cache if needed
}

// Callback invoked when received an SCSI command not in built-in list
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize) {
    void const *response = NULL;
    int32_t resplen = 0;

    switch (scsi_cmd[0]) {
        default:
            // Set Sense = Invalid Command Operation
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
            resplen = -1;
            break;
    }

    if (resplen > bufsize)
        resplen = bufsize;

    if (response && (resplen > 0)) {
        memcpy(buffer, response, resplen);
    }

    return resplen;
}

//--------------------------------------------------------------------+
// DEVICE CALLBACKS
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    // Device connected to host
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    cdc_connected = false;
    msc_connected = false;
    usb_audio_dsp_bridge_stop();
}

// Invoked when usb bus is suspended
void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
}

// Must be user-implemented
uint32_t tusb_time_millis_api(void) {
    return HAL_GetTick();
}
