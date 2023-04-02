/*
 * LED drive.c
 *
 * Created: 05.03.2023 15:21:50
 * Author : Bogusław
 */

// 1 MHz
#define F_CPU 100000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "timer.h"
#include "pwm.h"
#include "pin_io.h"
#include "main.h"

// setup initial device status
static inline void setup_initial_port_status();

int main(void)
{
    setup_initial_port_status();
    setup_timer_3ms();
    start_pwm();

    while (1)
    {
        sei();
        sleep_enable();
        sleep_cpu();
    }
}

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
    // 7 - WY led
    DIDR0 = _BV(2) | _BV(3) | _BV(7);

    // 0 - WE zapłon
    // 1 - WE rozrusznik
    // 2 - WY światła
    // 3 - WE reset
    DDRB = _BV(2);
    PORTB = _BV(3);
}

void initial_setup() __attribute__((naked, used, section(".init3")));

void initial_setup()
{
    // read reset cause
    MCUSR_initial_copy = MCUSR;
    MCUSR = 0;
    // reset watchdog
    // by default, it times out after 15 ms, which is fine
    wdt_reset();
    // setup clock prescaler
    // Table 6-11. Clock Prescaler Select
    // 0000 - no prescaler - 8 MHz
    // 0001 - /2           - 4 MHz
    // 0010 - /4           - 2 MHz
    // 0011 - /8           - 1 MHz
    // enable change possibility
    CLKPR = _BV(CLKPCE);
    // write actual value - 4MHz
    CLKPR = _BV(CLKPS0);
}
