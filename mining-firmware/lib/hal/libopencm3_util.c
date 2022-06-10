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

#include "libopencm3_util.h"
#ifdef IO_USB_CDC
#include "libopencm3_usb.h"
#endif

static void clock_setup(void) {

#ifdef STM32F4DISCOVERY
    rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
    rcc_periph_clock_enable(RCC_GPIOD);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART2);
    rcc_periph_clock_enable(RCC_OTGFS);
#endif
#ifdef STM32F0DISCOVERY
    rcc_clock_setup_in_hsi_out_48mhz();
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART2);
#endif
#ifdef STM32L4DISCOVERY_IOT01A
    rcc_osc_on(RCC_HSI16);
    rcc_osc_on(RCC_MSI);
    rcc_set_msi_range(RCC_CR_MSIRANGE_8MHZ);

    flash_prefetch_enable();
    flash_dcache_enable();
    flash_icache_enable();
    rcc_set_main_pll(RCC_PLLCFGR_PLLSRC_HSI16, 4, 40, RCC_PLLCFGR_PLLP_DIV7,
                     RCC_PLLCFGR_PLLQ_DIV2, RCC_PLLCFGR_PLLR_DIV2);
    rcc_osc_on(RCC_PLL);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOD);
    rcc_periph_clock_enable(RCC_USART1);
    rcc_periph_clock_enable(RCC_USART2);
    flash_set_ws(FLASH_ACR_LATENCY_4WS);
    rcc_set_sysclk_source(RCC_CFGR_SW_PLL); // 80 MHz
    rcc_wait_for_sysclk_status(RCC_PLL);
    rcc_ahb_frequency = 80000000;
    rcc_apb1_frequency = 80000000;
    rcc_apb2_frequency = 80000000;
#endif
}

static void gpio_setup(void) {

#ifdef IO_USB_CDC
    // setup USB
    gpio_mode_setup(USART_DEBUG_TXRX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    USART_USB_CDC_PIN);
    gpio_set_af(USART_DEBUG_TXRX_GPIO_PORT, USART_USB_CDC_GPIO_AF, USART_USB_CDC_PIN);
#endif
#ifdef IO_USART
    // setup USART_DATA
    gpio_mode_setup(USART_DATA_TXRX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    USART_DATA_TXRX_PIN);
    gpio_set_af(USART_DATA_TXRX_GPIO_PORT, USART_DATA_TXRX_GPIO_AF, USART_DATA_TXRX_PIN);
#endif
#ifdef USART_DEBUG
    // setup USART_DEBUG
    gpio_mode_setup(USART_DEBUG_TXRX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    USART_DEBUG_TXRX_PIN);
    gpio_set_af(USART_DEBUG_TXRX_GPIO_PORT, USART_DEBUG_TXRX_GPIO_AF,
                USART_DEBUG_TXRX_PIN);
#endif
    // setup LEDs
    gpio_mode_setup(LEDS_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_ALL);
}

static void systick_setup(void) {

    systick_set_reload(STK_RELOAD);
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_counter_enable();
    systick_interrupt_enable();
}

volatile uint32_t sys_count;
volatile uint32_t overflow_count;

void sleep(uint32_t delay) {

    uint32_t wake = sys_count + delay;
    while (wake > sys_count)
        ;
}

void leds_blink(uint16_t gpios, size_t duration) {
    for (size_t i = 0; i < duration; i++) {
        LED_TOGGLE(gpios);
        sleep(1);
        LED_TOGGLE(gpios);
        sleep(1);
    }
}

void usart_setup(void) {

#if defined(USART_DEBUG)
    usart_set_baudrate(USART_DEBUG, USART_DEBUG_BAUDRATE);
    usart_set_databits(USART_DEBUG, 8);
    usart_set_stopbits(USART_DEBUG, USART_STOPBITS_1);
    usart_set_mode(USART_DEBUG, USART_MODE_TX_RX);
    usart_set_parity(USART_DEBUG, USART_PARITY_NONE);
    usart_set_flow_control(USART_DEBUG, USART_FLOWCONTROL_NONE);
    usart_enable(USART_DEBUG);
#endif
#if defined(USART_DATA)
    usart_set_baudrate(USART_DATA, USART_DATA_BAUDRATE);
    usart_set_databits(USART_DATA, 8);
    usart_set_stopbits(USART_DATA, USART_STOPBITS_1);
    usart_set_mode(USART_DATA, USART_MODE_TX_RX);
    usart_set_parity(USART_DATA, USART_PARITY_NONE);
    usart_set_flow_control(USART_DATA, USART_FLOWCONTROL_NONE);
    usart_enable(USART_DATA);
    usart_enable_rx_interrupt(USART_DATA);
    nvic_enable_irq(USART_DATA_NVIQ_IRQ);
#endif
}

#if LOG_LEVEL > 0 && defined(USART_DEBUG)
int _write(int file, char *ptr, int len) {

    int i;
    (void)file;
    usart_wait_send_ready(USART_DEBUG);
    for (i = 0; i < len; i++) {
        if (ptr[i] == '\n') {
            usart_send_blocking(USART_DEBUG, '\r');
        }

        usart_send_blocking(USART_DEBUG, ptr[i]);
    }
    return i;
}
#endif

/**
 * @brief Loads a serial number string into given buffer.
 * @param[out] serial_number Pointer to the serial number buffer
 * @param len Size of serial number buffer in bytes
 */
void load_serial_number(char *serial_number, size_t len) {
#ifdef STM32
    // Generates a string of size 27 using the unique device ID register (96 bits),
    // formatted as 'UID(95:64)-UID(63:32)-UID(31:0)\0'
    snprintf(serial_number, len, "%08lx-%08lx-%08lx", DESIG_UNIQUE_ID2, DESIG_UNIQUE_ID1,
             DESIG_UNIQUE_ID0);
#endif
}

#ifdef IO_USART
volatile uint8_t buf[48];
volatile uint8_t *p = buf;

#ifdef STM32L4DISCOVERY_IOT01A
MAKE_USART_ISR(usart1)
#endif
#ifdef STM32F0DISCOVERY
MAKE_USART_ISR(usart2)
#endif

void board_send_data(const uint8_t *data, const int len) {
    int i;
    usart_wait_send_ready(USART_DATA);
    for (i = 0; i < len; i++) {
        usart_send_blocking(USART_DATA, data[i]);
    }
}
#endif

void sys_tick_handler(void) {
    sys_count++;
#ifdef IO_USART
    p = buf;
#endif
}
/**
 * @brief Handles all board initialization procedures, e.g. setup of clock, gpios etc.
 * @param[out] serial_number Pointer to serial number output buffer
 * @param sn_len Size of serial number buffer in bytes
 */
void libopencm3_board_setup(char *serial_number, size_t sn_len) {

    clock_setup();
    gpio_setup();
    systick_setup();
    usart_setup();
    load_serial_number(serial_number, sn_len);

#ifdef IO_USB_CDC
    // Enable interrupt OTG_FS
    nvic_enable_irq(NVIC_OTG_FS_IRQ);
    usbd_setup((const char *)serial_number, sn_len);
#endif
    sleep(1);
}
