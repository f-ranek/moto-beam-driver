/*
 * initial.c
 *
 * Created: 05.03.2023 16:45:02
 *  Author: Bogusław
 */

#include <avr/io.h>
#include <avr/wdt.h>

#include "functions.h"

void setup_initial_port_status() {
    // Analog Comparator Disable
    ACSR |= _BV(ACD);

    // input && internal pullup
    DDRA = 0;
    // 0 - przycisk
    // 1 - luz
    PORTA = _BV(0) | _BV(1)
        // io ports used for SPI
        | _BV(4) | _BV(6);

    // Digital Input Disable Register 0
    // 2 - we LED
    // 3 - sensor świateł
    // 5/7 - wy LED
    DIDR0 = _BV(ADC3D) | _BV(ADC2D) | _BV(ADC5D) | _BV(ADC7D);

    // TODO: przekonfigurować do wartości ostatecznych
    // 0 - zapłon
    // 1 - rozrusznik
    DDRB = 0;
    PORTB = _BV(0) | _BV(1) // to nam nie będzie potrzebne, bo będzie zewnętrzny pull-down
        | _BV(2) | _BV(3);
}

void setup_watchdog(void)
    __attribute__((naked))
    __attribute__((section(".init3")));

void setup_watchdog()
{
    wdt_reset();
    MCUSR = 0;
    WDTCSR |= _BV(WDCE) | _BV(WDE);
    // Table 8-3. Watchdog Timer Prescale Select, WDP3:0 = 2, 64 ms
    WDTCSR = _BV(WDE) | _BV(WDP1);
}
