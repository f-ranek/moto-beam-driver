/*
 * timer_3ms.c
 *
 * Created: 05.03.2023 16:34:49
 *  Author: Bogusław
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "timer.h"

// perform periodic application logic
extern void loop_application_logic();

void setup_timer_3ms()
{
    // enable interrupt on compare/match
    TIMSK1 |= _BV(OCIE1A);
    // count to 374 - magic value calculated from 1MHz timer and /8 prescaler
    // count to 1499 - magic value calculated from 4MHz timer and /8 prescaler
    // count to 374 - magic value calculated from 8MHz timer and /64 prescaler
    // f = f_CLK / (2 * N * (OCR1A+1)), where N = prescaler value

    // 1540 is calibrated value
    // TODO: read from EEPROM ;)
    OCR1A = 1540;

    // start counter

    // WGM = 4, CTC (Clear Timer on Compare), TOP = OCR1A
    // Table 12-6. Clock Select Bit Description
    // 001 - no prescaler
    // 010 - /8
    // 011 - /64
    // 100 - /256
    // 101 - /1024
    TCCR1B = _BV(WGM12) | _BV(CS11);
}

// maks 196 sekund
uint16_t __timer_3ms_counter;

// interrupt every 3 ms
ISR (TIM1_COMPA_vect, ISR_NOBLOCK)
{
    PORTA |= _BV(7);
    // do the job
    loop_application_logic();

    // increment timer tick
    ++__timer_3ms_counter;
    // reset watchdog
    wdt_reset();
    PORTA &= ~_BV(7);
}
