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
#include "string.h"
#include "util.h"
#include <sha256d.h>

MINING_CTX ctx;

static uint32_t *nonce = NONCE_PTR(ctx);
static uint8_t hash[32];

/**
 * @brief Callback that is executed by peripheral as soon as data of mining-software is
 *        fully received.
 * @param buf[in] Pointer to peripheral RX buffer
 * @param len Length of received data in bytes
 */
void board_read_data(const char *buf, const int len) {
    LOG_INFO("RX: received new data\n");
    memcpy(ctx.rx_buf, buf, len);
    LOG_DEBUG_HEX("RX buffer", ctx.rx_buf, 48);

    sha256d_ms_init(&ctx.sha256d_ms_ctx, (uint32_t *)ctx.rx_buf);
    ctx.nonce_cached = BSWAP32(*nonce);
    LOG_INFO("starting with nonce '0x%08lx'\n", ctx.nonce_cached);
    board_set_status(MINER_STATUS_PROCESSING);
}

/**
 * @brief Computes the block header hash and compares result with the target hash
 * @details Uses an adjusted difficulty to satisfy the low hash rate.
 *          Target hash (big endian) = 0000FF..FF
 * @return 0 iff header_hash <= target_hash
 */
static inline uint16_t check_hash(void) {
    ATOMIC_CONTEXT();
    sha256d_ms((uint32_t *)hash, &ctx.sha256d_ms_ctx);
    LOG_DEBUG("nonce: 0x%08lx\n", ctx.nonce_cached);
    LOG_DEBUG_HEX("sha256d_ms_ctx.data", (uint8_t *)ctx.sha256d_ms_ctx.data, 256);
    LOG_DEBUG_HEX("sha256d_ms_ctx.midstate", (uint8_t *)ctx.sha256d_ms_ctx.midstate, 32);
    LOG_DEBUG_HEX("sha256d_ms_ctx.preshash", (uint8_t *)ctx.sha256d_ms_ctx.prehash, 32);
    LOG_DEBUG_HEX("hash", hash, 32);
    return *(uint16_t *)(hash + 28);
}

/**
 * @brief main loop of miner that continuously performs block header hashing as long as
 *        the miner is in processing state.
 * @details On each iteration the nonce in the block header is incremented. Hashing is
 *          performed until nonce=2^32-1. Each valid nonce will be send back to the mining
 *          software. New work can be received anytime via an interrupt.
 */
static void __attribute__((__noreturn__)) scanhash_loop(void) {
    while (1) {
        if (board_get_status()) {
            if (*nonce == 0xFFFFFFFF) {
                board_set_status(MINER_STATUS_IDLE);
            } else if (check_hash() == 0) {
                LOG_INFO("<<<SUCCESS>> found nonce '0x%08lx'\n", BSWAP32(*nonce));
                LOG_INFO("hash = xxxx....xxxx%08lx\n", ((uint32_t *)hash)[7]);
                // sends nonce (big endian) back to the mining software
                board_send_data((uint8_t *)nonce, 4);
                board_showsuccess();
            }
            // bswap32 necessary since we want linear increment of starting nonce
            *nonce = BSWAP32(++ctx.nonce_cached);
        }
    }
}

int main(void) {

    board_setup(&ctx);
    BOARD_PRINT_WELCOME(&ctx, board_hashrate(check_hash));
    board_set_status(MINER_STATUS_IDLE);

    scanhash_loop();

    // unreachable code
    return 0;
}
