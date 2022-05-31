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

#include "unity_config.h"
#ifdef LIBOPENCM3
#include <libopencm3_util.h>
#endif

void unity_output_start(void) {
#ifdef LIBOPENCM3
    usart_setup();
#endif
}

void unity_output_char(char c) {
#ifdef LIBOPENCM3
    usart_wait_send_ready(UNITTEST_USART);
    if (c == '\n') {
        usart_send_blocking(UNITTEST_USART, '\r');
    }
    usart_send_blocking(UNITTEST_USART, c);
#endif
}

void unity_output_flush(void) {}

void unity_output_complete(void) {}
