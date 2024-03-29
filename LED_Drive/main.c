/*
 * LED drive.c
 *
 * Created: 05.03.2023 15:21:50
 * Author : Bogusław
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "timer.h"
#include "pwm.h"
#include "pin_io.h"
#include "main.h"
#include "application.h"

FUSES =
{
    // SUT = 00 - startup time = 6 CK
    .low = (FUSE_CKDIV8 & FUSE_SUT1 & FUSE_SUT0 & FUSE_CKSEL3 & FUSE_CKSEL2 & FUSE_CKSEL0),
    // BODLEVEL = 100 - ~4.3V
    .high = (FUSE_SPIEN & FUSE_WDTON & FUSE_BODLEVEL1 & FUSE_BODLEVEL0),
    .extended = EFUSE_DEFAULT,
};

#ifdef SIMULATION

int main(void)
{
    start_pwm();
    init_application();

    while (1)
    {
        loop_application_logic();
        ++__timer_3ms_counter;
        wdt_reset();
        sei();
    }
}
#else // !SIMULATION
int main(void)
{
    setup_timer_3ms();
    start_pwm();
    init_application();

    while (1)
    {
        sei();
        sleep_enable();
        sleep_cpu();
    }
}
#endif // SIMULATION

// setup initial device status
static inline __attribute__ ((always_inline)) void setup_initial_port_status() {
    // Analog Comparator Disable
    ACSR |= _BV(ACD);

    // 0 - WE przycisk

    // SPI interface
    // 4 - SCK
    // 5 - DO - data out

    // 6 - morse debug bit, 1 - off, 0 - on
    // 7 - LED, 0 - off, 1 - on

    // bit 1 to mark as output
    DDRA = _BV(4) | _BV(5) | _BV(6) | _BV(7);
    // bit 1 to enable internal pullup on input, OUT on output
    PORTA = _BV(0) | _BV(6);

    // Digital Input Disable Register 0
    // 1 - WE czujnik zmierzchu
    // 2 - WE aku
    // 3 - WE sensor świateł
    DIDR0 = _BV(1) | _BV(2) | _BV(3);

    // 0 - WE olej
    // 1 - WE luz
    // 2 - WY światła, 1 - off, 0 - on
    // 3 - WE reset

    // enable pull-up for bits 0 and 1
    PORTB = _BV(0) | _BV(1) | _BV(2);
    // mark bit 2 as output, it will have 1 on the pin
    DDRB = _BV(2);
}

void initial_setup() __attribute__((naked, used, section(".init3")));

void initial_setup()
{
    // read reset cause
    MCUSR_initial_copy = MCUSR;
    MCUSR = 0;

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

    setup_initial_port_status();

    // reset watchdog
    // by default, it times out after 15 ms, which is fine
    wdt_reset();
}
