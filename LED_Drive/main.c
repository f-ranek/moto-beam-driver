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

#ifdef SIMULATION

// perform periodic application logic
extern void loop_application_logic();

int main(void)
{
    setup_initial_port_status();
    start_pwm();

    while (1)
    {
        loop_application_logic();
        ++__timer_3ms_counter;
        wdt_reset();
    }
}
#else // !SIMULATION
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
#endif // SIMULATION

void setup_initial_port_status() {
    // Analog Comparator Disable
    ACSR |= _BV(ACD);

    // input && internal pullup
    DDRA = _BV(7);
    // 0 - WE przycisk
    PORTA = _BV(0)
        // io ports used for SPI
        | _BV(4) | _BV(5) | _BV(6);

    // Digital Input Disable Register 0
    // 1 - WE czujnik zmierzchu
    // 2 - WE aku
    // 3 - WE sensor świateł
    // 7 - WY led
    DIDR0 = _BV(1) | _BV(2) | _BV(3) | _BV(7);

    // 0 - WE olej
    // 1 - WE luz
    // 2 - WY światła
    // 3 - WE reset
    DDRB = _BV(2);
    PORTB = _BV(0) | _BV(2) | _BV(1) | _BV(3);


    // SPI WIP
    // 4 - SCK
    // 5 - DO - data out
    // 6 - DI - data in
    DDRA |= _BV(4) | _BV(5);
    PORTA &= ~(_BV(4) | _BV(5));

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
