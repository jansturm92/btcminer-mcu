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

#ifndef MINING_FIRMWARE_SRC_UTIL_H
#define MINING_FIRMWARE_SRC_UTIL_H

#include "sha256d.h"
#include "stdio.h"

#if LOG_LEVEL > 0
#define PRINT_HEX(name, buf, len)                                                        \
    do {                                                                                 \
        printf("%s = ", name);                                                           \
        for (int i = 0; i < len; i++) {                                                  \
            printf("%02x", (buf)[i]);                                                    \
        }                                                                                \
        printf("\n");                                                                    \
    } while (0)
#define LOG_FMT(lvl, f_, ...)                                                            \
    do {                                                                                 \
        printf("[%s]    ", lvl);                                                         \
        printf((f_), ##__VA_ARGS__);                                                     \
    } while (0)

#define LOG_FMT_HEX(lvl, s, c, len)                                                      \
    do {                                                                                 \
        printf("[%s]    ", lvl);                                                         \
        PRINT_HEX(s, c, len);                                                            \
    } while (0)

#define LOG(f_, ...) printf((f_), ##__VA_ARGS__)
#define LOG_CYC(s, cyc) print_cyc(s, cyc)
#define BOARD_PRINT_WELCOME(ctx, hashrate) board_print_welcome(ctx, hashrate)
#if LOG_LEVEL == 1
#define LOG_DEBUG(f_, ...)
#define LOG_INFO(f_, ...) LOG_FMT("INFO", (f_), ##__VA_ARGS__)
#define LOG_DEBUG_HEX(s, c, len)
#define LOG_INFO_HEX(s, c, len) LOG_FMT_HEX("INFO", s, c, len)
#else
#define LOG_DEBUG(f_, ...) LOG_FMT("DEBUG", (f_), ##__VA_ARGS__)
#define LOG_INFO(f_, ...) LOG_FMT(" INFO", (f_), ##__VA_ARGS__)
#define LOG_DEBUG_HEX(s, c, len) LOG_FMT_HEX("DEBUG", s, c, len)
#define LOG_INFO_HEX(s, c, len) LOG_FMT_HEX(" INFO", s, c, len)
#endif
#else
#define LOG(f_, ...)
#define LOG_DEBUG(f_, ...)
#define LOG_INFO(f_, ...)
#define LOG_CYC(s, cyc)
#define LOG_DEBUG_HEX(s, c, len)
#define LOG_INFO_HEX(s, c, len)
#define BOARD_PRINT_WELCOME(ctx, hashrate)
#endif

typedef struct {
    char serial_number[32];

    // rx_buf = [midstate (32) | timestamp (4) | bits (4) | nonce (4) | reserved (48)]
    uint8_t rx_buf[96];
    uint32_t nonce_cached;
    SHA256D_MS_CTX sha256d_ms_ctx;
} MINING_CTX;

#define NONCE_PTR(ctx) &((ctx).sha256d_ms_ctx.data[3])
#define BSWAP32(v) __builtin_bswap32(v)

#endif // MINING_FIRMWARE_SRC_UTIL_H
