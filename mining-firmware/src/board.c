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

#include "board.h"

/**
 * @brief Handles all board initialization procedures, e.g. setup of clock, gpios etc.
 * @param ctx The mining context
 */
void board_setup(MINING_CTX *ctx) {
#ifdef LIBOPENCM3
    libopencm3_board_setup(ctx->serial_number, sizeof(ctx->serial_number));
#endif
}

/**
 * @brief Sets the status of the miner.
 * @param status #MINER_STATUS_IDLE or #MINER_STATUS_PROCESSING
 */
void board_set_status(int status) {
#ifdef LIBOPENCM3
    status == MINER_STATUS_PROCESSING ? LED_SET(LED_PROCESSING)
                                      : LED_CLEAR(LED_PROCESSING);
#endif
}

/**
 * @brief Returns the status of the miner.
 * @return #MINER_STATUS_IDLE or #MINER_STATUS_PROCESSING
 */
int board_get_status(void) {
#ifdef LIBOPENCM3
    return LED_READ(LED_PROCESSING);
#endif
}

/// Shows a visual effect, if supported by board.
void board_showsuccess(void) {
#if defined(LIBOPENCM3) && defined(LED_SUCCESS)
    leds_blink(LED_SUCCESS, 6);
#endif
}

#ifdef USART_DEBUG
/**
 * @brief Calculates the time it takes to compute the block header hash.
 * @param check_hash callback function used for block header hash computation.
 * @return uint32_t number of hashes per second
 */
uint32_t board_hashrate(uint16_t (*check_hash)(void)) {
#if defined(LIBOPENCM3)
    uint32_t cyc;
    MEASURE(cyc, check_hash());
    return SYSTEMCLOCK / cyc;
#endif
}

/**
 * @brief Prints information about the specific board, if LOG_LEVEL > 0.
 * @param ctx the mining context
 * @param hashrate hashrate of board
 */
void board_print_welcome(MINING_CTX *ctx, uint32_t hashrate) {

    LOG("\n***************************************************\n");
#if LOG_LEVEL == 2
    LOG("\tMCU Bitcoin Miner [DEBUG MODE]\n\n");
#else
    LOG("\tMCU Bitcoin Miner\n\n");
#endif
    LOG("Framework:\n"
        "\t%s\n",
        PIO_FRAMEWORK);
    LOG("Board:\n"
        "\tName: %s\n"
        "\tUID: %s\n",
        BOARD_NAME, ctx->serial_number);
#ifdef IO_USB_CDC
    LOG("USB Descriptor:\n"
        "\tidVendor: 0x%04x (%s)\n"
        "\tidProduct: 0x%04x (%s)\n"
        "\tiManufacturer: %s\n"
        "\tiProduct: %s\n"
        "\tiSerial: %s\n",
        USB_VENDOR_ID, USB_VENDOR_STRING, USB_PRODUCT_ID, USB_PRODUCT_STRING, BOARD_NAME,
        USB_DESCRIPTOR_IPRODUCT, ctx->serial_number);
#endif
    LOG("Mining:\n"
        "\tHashrate: %lu Hashes/s\n"
        "\tTargethash: %s\n",
        hashrate, "0000FFFF....FFFFF");
    LOG("\n***************************************************\n\n");
}
#endif
