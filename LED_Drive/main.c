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
#include "main.h"

// setup initial device status
static inline void setup_initial_port_status();

int main(void)
{
    setup_initial_port_status();
    setup_timer_3ms();
    start_pwm();

    // TODO: DEBUG XXX
    //PINA = 0x03; // gear and no button pressed
    //PINB = 0x03; // ignition power on
    //MCUSR_initial_copy = _BV(EXTRF); // external reset

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

void setup_watchdog() __attribute__((naked, used, section(".init3")));

void setup_watchdog()
{
    MCUSR_initial_copy = MCUSR;
    MCUSR = 0;
    wdt_reset();
    WDTCSR |= _BV(WDCE) | _BV(WDE);
    // Table 8-3. Watchdog Timer Prescale Select, WDP3:0 = 2, 64 ms
    WDTCSR = _BV(WDE) | _BV(WDP1);
}
