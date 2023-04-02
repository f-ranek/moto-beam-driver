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
    // f = f_CLK / (2 * N * (OCR1A+1)), where N = prescaler value
    OCR1A = 374;

    // XXX TODO DEBUG
    // OCR1A = 3;

    // start counter

    // WGM = 4, CTC (Clear Timer on Compare), TOP = OCR1A
    // use clock source with prescaler /8
    TCCR1B = _BV(WGM12) | _BV(CS11);
    // use clock source with prescaler /64
    //TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10);
    // use clock source with prescaler /256
    //TCCR1B = _BV(WGM12) | _BV(CS12);
    // use clock source with prescaler /1024
    //TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS10);

}

// maks 196 sekund
uint16_t __timer_3ms_counter;

// interrupt every 3 ms
ISR (TIM1_COMPA_vect, ISR_NAKED)
{
    // do the job
    loop_application_logic();

    // increment timer tick
    if (++__timer_3ms_counter == 0) {
        uint8_t hight_bit = __timer_196s_counter & _BV(7);
        __timer_196s_counter = (__timer_196s_counter+1) | hight_bit;
    }

    // reset watchdog
    wdt_reset();

    // exit interrupt handler
    reti();
}
