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
    DDRA = _BV(7);
    // 0 - WE przycisk
    // 1 - WE luz
    PORTA = _BV(0) | _BV(1)
        // io ports used for SPI
        | _BV(4) | _BV(5) | _BV(6);

    // Digital Input Disable Register 0
    // 2 - WE led
    // 3 - WE sensor świateł
    // 7 - WY led, ew. 5
    DIDR0 = _BV(2) | _BV(3) | _BV(7);

    // 0 - WE zapłon
    // 1 - WE rozrusznik
    // 2 - WY światła
    // 3 - WE reset
    DDRB = _BV(2);
    PORTB = _BV(3);
}

void setup_watchdog(void)
    __attribute__((naked))
    __attribute__((section(".init3")));

void setup_watchdog()
{
    wdt_reset();
    // MCUSR = 0; -- po co to?
    WDTCSR |= _BV(WDCE) | _BV(WDE);
    // Table 8-3. Watchdog Timer Prescale Select, WDP3:0 = 2, 64 ms
    WDTCSR = _BV(WDE) | _BV(WDP1);
}
