// Copyright (C) 2022 Jan Sturm
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.

#include <sha256d.h>
#include <stm32_util.h>
#include <usb-cdc.h>

#define USB_DESCRIPTOR_IPRODUCT "Bitcoin USB Miner"

SHA256D_MS_CTX sha256d_ms_ctx;

/// rx_buf = [midstate (32B) | timestamp (4B) | bits (4B) | nonce (4B) | reserved (48B)]
static uint8_t rx_buf[96];
static uint32_t *nonce = &(sha256d_ms_ctx.data[3]);
static uint32_t nonce_cached;
static uint8_t hash[32];

static usbd_device *usb_device;

static char serial_number[27];

static const struct usb_device_descriptor dev_descriptor = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_CDC,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = STM32_USB_VENDOR_ID,
    .idProduct = STM32_USB_PRODUCT_ID,
    .bcdDevice = 0x0200,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

/// USB string descriptors
static const char *usb_strings[] = {
    STM32_BOARD_NAME,        // iManufacturer
    USB_DESCRIPTOR_IPRODUCT, // iProduct
    serial_number,           // iSerialNumber
};

/// USB interrupt handler
// cppcheck-suppress unusedFunction
void otg_fs_isr(void) { usbd_poll(usb_device); }

/**
 * @brief RX callback for CDC device, that reads data from mining software.
 * @details Writes 48 bytes into #rx_buf buffer. Subsequently performs sha256d midstate
 *          precomputations and changes the status of the miner to 'processing'.
 * @param usbd_dev the usb device handle returned from usbd_setup()
 * @param ep unused
 */
static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep) {
    (void)ep;

    // read 48 bytes = [midstate (32B) | timestamp (4B) | bits (4B) | nonce (4B)]
    usbd_ep_read_packet(usbd_dev, 0x01, rx_buf, 48);
    LOG_INFO("USB: received new data\n");
    LOG_DEBUG_HEX("RX buffer", rx_buf, 48);

    sha256d_ms_init(&sha256d_ms_ctx, (uint32_t *)rx_buf);
    nonce_cached = BSWAP32(*nonce);
    LOG_INFO("starting with nonce '0x%08lx'\n", nonce_cached);
    STM32_LED_SET(STM32_LED_PROCESSING);
}

/// sends nonce (big endian) back to the mining software
static void reply_nonce(void) {
    while (usbd_ep_write_packet(usb_device, 0x82, (uint8_t *)nonce, 4) == 0)
        ;
}

/**
 * @brief Computes the block header hash and compares result with the target hash
 * @details Uses an adjusted difficulty to satisfy the low hash rate.
 *          Target hash (big endian) = 0000FF..FF
 * @return 0 iff header_hash <= target_hash
 */
static inline uint16_t check_hash(void) {
    sha256d_ms((uint32_t *)hash, &sha256d_ms_ctx);
    LOG_DEBUG("nonce: 0x%08lx\n", nonce_cached);
    LOG_DEBUG_HEX("sha256d_ms_ctx.data", (uint8_t *)sha256d_ms_ctx.data, 256);
    LOG_DEBUG_HEX("sha256d_ms_ctx.midstate", (uint8_t *)sha256d_ms_ctx.midstate, 32);
    LOG_DEBUG_HEX("sha256d_ms_ctx.preshash", (uint8_t *)sha256d_ms_ctx.prehash, 32);
    LOG_DEBUG_HEX("hash", hash, 32);
    return *(uint16_t *)(hash + 28);
}

/**
 * @brief main loop of miner that continuously performs block header hashing as long as
 *        the miner is in processing state.
 * @details On each iteration the nonce in the block header is incremented and hashing is
 *          performed until a valid nonce is found or nonce=2^32-1. New work from the
 *          mining software can be received anytime because of the USB interrupt.
 */
static void __attribute__((__noreturn__)) scanhash_loop(void) {
    while (1) {
        if (STM32_LED_READ(STM32_LED_PROCESSING)) {
            if (*nonce == 0xFFFFFFFF || check_hash() == 0) {
                LOG_INFO("<<<SUCCESS>> found nonce '0x%08lx'\n", BSWAP32(*nonce));
                LOG_INFO("hash = xxxx....xxxx%08lx\n", ((uint32_t *)hash)[7]);
                STM32_LED_CLEAR(STM32_LED_PROCESSING);
                reply_nonce();
                stm32_leds_blink(STM32_LED_GREEN, 6);
            }
            // bswap32 necessary since we want linear increment of starting nonce
            *nonce = BSWAP32(++nonce_cached);
        }
    }
}

/**
 * @brief Calculates the time it takes to compute the block header hash. Uses SysTick to
 *        measure the cycle count.
 * @return uint32_t number of hashes per second
 */
static uint32_t measure_hashrate(void) {
    uint32_t cyc;
    MEASURE(cyc, check_hash());
    return STM32_SYSTEMCLOCK / cyc;
}

/**
 * @brief Loads a serial number string into #serial_number buffer.
 * @details Generates a string of size #SERIAL_NUMBER_LEN using the unique device ID
 *          register (96 bits), formatted as 'UID(95:64)-UID(63:32)-UID(31:0)\0'
 */
static void load_serial_number(void) {
    snprintf(serial_number, sizeof(serial_number), "%08lx-%08lx-%08lx", DESIG_UNIQUE_ID2,
             DESIG_UNIQUE_ID1, DESIG_UNIQUE_ID0);
}

static void print_welcome(void) {

    LOG("\n***************************************************\n");
#if LOG_LEVEL == 2
    LOG("\tSTM32 USB Bitcoin Miner [DEBUG MODE]\n\n");
#else
    LOG("\tSTM32 USB Bitcoin Miner\n\n");
#endif
    LOG("Framework:\n"
        "\t%s\n",
        PIO_FRAMEWORK);
    LOG("Board:\n"
        "\tName: %s\n"
        "\tUID: %s\n",
        STM32_BOARD_NAME, serial_number);
    LOG("USB Descriptor:\n"
        "\tidVendor: 0x%04x (%s)\n"
        "\tidProduct: 0x%04x (%s)\n"
        "\tiManufacturer: %s\n"
        "\tiProduct: %s\n"
        "\tiSerial: %s\n",
        STM32_USB_VENDOR_ID, STM32_USB_VENDOR_STRING, STM32_USB_PRODUCT_ID,
        STM32_USB_PRODUCT_STRING, STM32_BOARD_NAME, USB_DESCRIPTOR_IPRODUCT,
        serial_number);
    LOG("Mining:\n"
        "\tHashrate: %lu Hashes/s\n"
        "\tTargethash: %s\n",
        measure_hashrate(), "0000FFFF....FFFFF");
    LOG("\n***************************************************\n\n");
}

int main(void) {

    stm32_board_setup();
    load_serial_number();
    print_welcome();

    usb_device = usbd_setup(&dev_descriptor, cdcacm_data_rx_cb, usb_strings);

    STM32_LED_CLEAR(STM32_LED_PROCESSING);
    scanhash_loop();

    // unreachable code
    return 0;
}
