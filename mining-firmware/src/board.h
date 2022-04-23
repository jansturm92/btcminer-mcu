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

#ifndef MINING_FIRMWARE_SRC_BOARD_H
#define MINING_FIRMWARE_SRC_BOARD_H

#include "util.h"
#ifdef LIBOPENCM3
#include "libopencm3_util.h"
#else
#error "Choose a framework"
#endif

#define MINER_STATUS_IDLE 0
#define MINER_STATUS_PROCESSING 1

void board_setup(MINING_CTX *ctx);
void board_set_status(int);
int board_get_status(void);
void board_showsuccess(void);

#ifdef USART_DEBUG
uint32_t board_hashrate(uint16_t (*check_hash)(void));
void board_print_welcome(MINING_CTX *ctx, uint32_t hashrate);
#endif

#endif // MINING_FIRMWARE_SRC_BOARD_H
