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

#ifndef MINING_FIRMWARE_TEST_UNITEST_TRANSPORT_H
#define MINING_FIRMWARE_TEST_UNITEST_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBOPENCM3
#include <libopencm3_util.h>
#endif

void unittest_uart_begin(void);
void unittest_uart_putchar(char c);
void unittest_uart_flush(void);
void unittest_uart_end(void);

#ifdef __cplusplus
}
#endif

#endif // MINING_FIRMWARE_TEST_UNITEST_TRANSPORT_H
