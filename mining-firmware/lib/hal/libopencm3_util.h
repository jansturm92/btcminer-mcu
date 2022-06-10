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

#ifndef MINING_FIRMWARE_LIB_HAL_LIBOPENCM3_UTIL_H
#define MINING_FIRMWARE_LIB_HAL_LIBOPENCM3_UTIL_H

#define PIO_FRAMEWORK "libopencm3"

#include "libopencm3/cm3/cortex.h"
#include <libopencm3/cm3/common.h>
#include <libopencm3/cm3/systick.h>
#include <stdint.h>
#include <stdio.h>

#define USB_DESCRIPTOR_IPRODUCT "Bitcoin USB Miner"
#define ATOMIC_CONTEXT CM_ATOMIC_CONTEXT

#ifdef STM32
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#define USB_VENDOR_ID 0x0483
#define USB_VENDOR_STRING "STMicroelectronics"
#define USB_PRODUCT_ID 0x5740
#define USB_PRODUCT_STRING "Virtual COM Port"
#endif

#ifdef STM32F4DISCOVERY
#include <libopencm3/stm32/f4/nvic.h>
#define BOARD_NAME "STM32F4DISCOVERY"
#define SYSTEMCLOCK 168000000 // Hz
#define USART_DEBUG USART2
#define USART_DEBUG_BAUDRATE 115200
#define USART_DEBUG_TXRX_GPIO_PORT GPIOA
#define USART_DEBUG_TXRX_GPIO_AF GPIO_AF7
#define USART_DEBUG_TXRX_PIN (GPIO2 | GPIO3)
#define USART_USB_CDC_GPIO_AF GPIO_AF10
#define USART_USB_CDC_PIN (GPIO11 | GPIO12)
#define LEDS_GPIO_PORT GPIOD
#define LED_GREEN GPIO12
#define LED_ORANGE GPIO13
#define LED_RED GPIO14
#define LED_BLUE GPIO15
#define LED_ALL (GPIO12 | GPIO13 | GPIO14 | GPIO15)
#define LED_PROCESSING LED_BLUE
#define LED_SUCCESS LED_GREEN
#define UNITTEST_INITIAL_SLEEP_DURATION 40
#define UNITTEST_USART USART_DEBUG
#endif
#ifdef STM32L4DISCOVERY_IOT01A
#include <libopencm3/stm32/l4/nvic.h>
#define BOARD_NAME "STM32L4DISCOVERY_IOT01A"
#define SYSTEMCLOCK 80000000 // Hz
#define USART_DEBUG USART2
#define USART_DEBUG_BAUDRATE 115200
#define USART_DEBUG_TXRX_GPIO_PORT GPIOD
#define USART_DEBUG_TXRX_GPIO_AF GPIO_AF7
#define USART_DEBUG_TXRX_PIN (GPIO5 | GPIO6)
#define USART_DATA USART1
#define USART_DATA_NVIQ_IRQ NVIC_USART1_IRQ
#define USART_DATA_BAUDRATE 9600
#define USART_DATA_TXRX_GPIO_PORT GPIOB
#define USART_DATA_TXRX_GPIO_AF GPIO_AF7
#define USART_DATA_TXRX_PIN (GPIO6 | GPIO7)
#define LEDS_GPIO_PORT GPIOB
#define LED_GREEN GPIO14
#define LED_ALL GPIO14
#define LED_PROCESSING LED_GREEN
#define UNITTEST_INITIAL_SLEEP_DURATION 10
#define UNITTEST_USART USART_DEBUG
#endif
#ifdef STM32F0DISCOVERY
#include <libopencm3/stm32/f0/nvic.h>
#define BOARD_NAME "STM32F0DISCOVERY"
#define USART_DATA USART2
#define USART_DATA_NVIQ_IRQ NVIC_USART2_IRQ
#define USART_DATA_BAUDRATE 38400
#define USART_DATA_TXRX_GPIO_PORT GPIOA
#define USART_DATA_TXRX_GPIO_AF GPIO_AF1
#define USART_DATA_TXRX_PIN (GPIO2 | GPIO3)
#define LEDS_GPIO_PORT GPIOC
#define LED_BLUE GPIO8
#define LED_GREEN GPIO9
#define LED_ALL (GPIO8 | GPIO9)
#define LED_PROCESSING LED_BLUE
#define LED_SUCCESS LED_GREEN
#define UNITTEST_INITIAL_SLEEP_DURATION 1
#define UNITTEST_USART USART_DATA
#endif

#define LED_SET(gpios) gpio_set(LEDS_GPIO_PORT, gpios)
#define LED_CLEAR(gpios) gpio_clear(LEDS_GPIO_PORT, gpios)
#define LED_READ(gpios) gpio_get(LEDS_GPIO_PORT, gpios)
#define LED_TOGGLE(gpios) gpio_toggle(LEDS_GPIO_PORT, gpios)

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

#define MAKE_USART_ISR(usart)                                                            \
    void usart##_isr(void) {                                                             \
        LED_CLEAR(LED_PROCESSING);                                                       \
        *p++ = usart_recv(USART_DATA);                                                   \
        if (p == buf + 48) {                                                             \
            board_read_data((const char *)buf, sizeof(buf));                             \
            p = buf;                                                                     \
        }                                                                                \
    }

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void leds_blink(uint16_t gpios, size_t duration);
void sleep(uint32_t delay);
void usart_setup(void);
void load_serial_number(char *serial_number, size_t len);
void libopencm3_board_setup(char *serial_number, size_t len);
extern void board_read_data(const char *buf, int len);
void board_send_data(const uint8_t *data, int len);

#if LOG_LEVEL > 0
int _write(int file, char *ptr, int len);
#endif

#endif // MINING_FIRMWARE_LIB_HAL_LIBOPENCM3_UTIL_H
