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

#include "stm32_util.h"

static void clock_setup(void) {

    rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
    rcc_periph_clock_enable(RCC_GPIOD);
    rcc_periph_clock_enable(RCC_RNG);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART2);
    rcc_periph_clock_enable(RCC_OTGFS);
}

static void gpio_setup(void) {

    gpio_mode_setup(STM32_TXRX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, STM32_TXRX_PIN);
    gpio_set_af(STM32_TXRX_GPIO_PORT, STM32_TXRX_GPIO_AF, STM32_TXRX_PIN);
    gpio_mode_setup(STM32_LEDS_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    STM32_LED_ALL);

    gpio_mode_setup(STM32_TXRX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
    gpio_set_af(STM32_TXRX_GPIO_PORT, GPIO_AF10, GPIO11 | GPIO12);
}

static void systick_setup(void) {

    systick_set_reload(STK_RELOAD);
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_counter_enable();
    systick_interrupt_enable();
}

volatile uint32_t sys_count;
volatile uint32_t overflow_count;

void sys_tick_handler(void) { sys_count++; }

void sleep(uint32_t delay) {

    uint32_t wake = sys_count + delay;
    while (wake > sys_count)
        ;
}

void stm32_leds_blink(uint16_t gpios, size_t duration) {
    for (size_t i = 0; i < duration; i++) {
        STM32_LED_TOGGLE(gpios);
        sleep(1);
        STM32_LED_TOGGLE(gpios);
        sleep(1);
    }
}

#if defined(UNIT_TEST) || LOG_LEVEL > 0
void usart_setup(void) {

    usart_set_baudrate(STM32_USART, STM32_USART_BAUDRATE);
    usart_set_databits(STM32_USART, 8);
    usart_set_stopbits(STM32_USART, USART_STOPBITS_1);
    usart_set_mode(STM32_USART, USART_MODE_TX_RX);
    usart_set_parity(STM32_USART, USART_PARITY_NONE);
    usart_set_flow_control(STM32_USART, USART_FLOWCONTROL_NONE);
    usart_enable(STM32_USART);
}
#endif

#if LOG_LEVEL > 0
int _write(int file, char *ptr, int len) {

    int i;
    (void)file;
    usart_wait_send_ready(STM32_USART);
    for (i = 0; i < len; i++) {
        if (ptr[i] == '\n') {
            usart_send_blocking(STM32_USART, '\r');
        }
        usart_send_blocking(STM32_USART, ptr[i]);
    }
    return i;
}

void print_cyc(const char *s, uint32_t cyc) { printf("\t--> %s: %lu cyc\n", s, cyc); }

void print_hex(const char *s, const unsigned char *c, int len) {

    printf("%s = ", s);
    for (int i = 0; i < len; i++) {
        printf("%02x", c[i]);
    }
    printf("\n");
}

#endif

void stm32_board_setup(void) {

    clock_setup();
    gpio_setup();
    systick_setup();
#if LOG_LEVEL > 0
    usart_setup();
#endif
    // Enable interrupt OTG_FS
    nvic_enable_irq(NVIC_OTG_FS_IRQ);

    sleep(1);
}
