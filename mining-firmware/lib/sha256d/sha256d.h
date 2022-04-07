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

#ifndef MINING_FIRMWARE_LIB_SHA256D_H
#define MINING_FIRMWARE_LIB_SHA256D_H

#include <stdint.h>

typedef struct {
    uint32_t data[64];
    uint32_t prehash[8];
    uint32_t *midstate;
} SHA256D_MS_CTX;

void sha256d(uint8_t *hash, const uint8_t *data, int len);

void sha256d_ms_init(SHA256D_MS_CTX *ctx, uint32_t *buf);
void sha256d_ms(uint32_t *hash, SHA256D_MS_CTX *ctx);

#endif // MINING_FIRMWARE_LIB_SHA256D_H
