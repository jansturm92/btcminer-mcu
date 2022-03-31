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

#include "unittest_transport.h"
#include <libopencm3/stm32/usart.h>
#include <stm32_util.h>

void unittest_uart_begin(void) { usart_setup(); }

void unittest_uart_putchar(char c) {

    usart_wait_send_ready(STM32_USART);
    if (c == '\n') {
        usart_send_blocking(STM32_USART, '\r');
    }
    usart_send_blocking(STM32_USART, c);
}

void unittest_uart_flush(void) {}

void unittest_uart_end(void) {}
