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

#ifndef MINING_FIRMWARE_LIB_STM32_UTIL_H
#define MINING_FIRMWARE_LIB_STM32_UTIL_H

#include <libopencm3/cm3/common.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/rng.h>
#include <libopencm3/stm32/usart.h>
#include <stdint.h>
#include <stdio.h>

#define PIO_FRAMEWORK "libopencm3"

#define STM32_USB_VENDOR_ID 0x0483
#define STM32_USB_VENDOR_STRING "STMicroelectronics"
#define STM32_USB_PRODUCT_ID 0x5740
#define STM32_USB_PRODUCT_STRING "Virtual COM Port"
#define STM32_BOARD_NAME "STM32F4DISCOVERY"

#define STM32_SYSTEMCLOCK 168000000 // Hz

#define STM32_USART USART2
#define STM32_USART_BAUDRATE 115200
#define STM32_UNITTEST_INITIAL_SLEEP_DURATION 40
#define STM32_TXRX_GPIO_PORT GPIOA
#define STM32_TXRX_GPIO_AF GPIO_AF7
#define STM32_TXRX_PIN (GPIO2 | GPIO3)
#define STM32_LEDS_GPIO_PORT GPIOD
#define STM32_LED_GREEN GPIO12
#define STM32_LED_ORANGE GPIO13
#define STM32_LED_RED GPIO14
#define STM32_LED_BLUE GPIO15
#define STM32_LED_ALL (GPIO12 | GPIO13 | GPIO14 | GPIO15)
#define STM32_LED_PROCESSING STM32_LED_BLUE
#define STM32_LED_SUCCESS STM32_LED_GREEN
#define STM32_LED_SET(gpios) gpio_set(STM32_LEDS_GPIO_PORT, gpios)
#define STM32_LED_CLEAR(gpios) gpio_clear(STM32_LEDS_GPIO_PORT, gpios)
#define STM32_LED_READ(gpios) gpio_get(STM32_LEDS_GPIO_PORT, gpios)
#define STM32_LED_TOGGLE(gpios) gpio_toggle(STM32_LEDS_GPIO_PORT, gpios)

#define STK_RELOAD 0x00FFFFFF
extern volatile uint32_t overflow_count;
extern volatile uint32_t sys_count;

#define MEASURE(res_cyc, func)                                                           \
    overflow_count = sys_count;                                                          \
    STK_CVR = 0;                                                                         \
    func;                                                                                \
    cyc = STK_CVR;                                                                       \
    overflow_count = sys_count - overflow_count;                                         \
    (res_cyc) = (overflow_count + 1) * STK_RELOAD - cyc

#define BSWAP32(v) __builtin_bswap32(v)

void stm32_leds_blink(uint16_t gpios, size_t duration);
void sleep(uint32_t delay);
void usart_setup(void);
void stm32_board_setup(void);

#if LOG_LEVEL > 0
int _write(int file, char *ptr, int len);
void print_cyc(const char *s, uint32_t cyc);
void print_hex(const char *s, const unsigned char *c, int len);
#define LOG_FMT(lvl, f_, ...)                                                            \
    do {                                                                                 \
        printf("[%s]    ", lvl);                                                         \
        printf((f_), ##__VA_ARGS__);                                                     \
    } while (0)

#define LOG_FMT_HEX(lvl, s, c, len)                                                      \
    do {                                                                                 \
        printf("[%s]    ", lvl);                                                         \
        print_hex(s, c, len);                                                            \
    } while (0)

#define LOG(f_, ...) printf((f_), ##__VA_ARGS__)
#define LOG_CYC(s, cyc) print_cyc(s, cyc)
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
#endif

#endif // MINING_FIRMWARE_LIB_STM32_UTIL_H
