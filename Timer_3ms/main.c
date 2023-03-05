/*
 * Timer_3ms.c
 *
 * Created: 11.02.2023 17:47:21
 * Author : Bogusław
 */

// 1 MHz
#define F_CPU 100000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
// TODO: fuse.h i ustawienia uC


uint8_t j = 0;

// interrupt every 3 ms
ISR (TIM1_COMPA_vect)
{
    // do the job
    j++;
    PORTA = j;
}

int main(void)
{
    // make port B output
	DDRB = 0x7;
    // make port A output
    DDRA = 0xFF;
    // setup timer mode

    // enable interrupt on compare/match
    TIMSK1 |= 1 << OCIE1A;
    // enable interrupt on overflow
    //TIMSK1 |= 1 << TOIE1;
    // count to 374
    OCR1A = 374;

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

    // f = f_CLK / (2 * N * (OCR1A+1)), where N = prescaler value


// todo: setup watchdog
// TODO: ustawić nieużywane piny jako WE + pullup

    sei();
    sleep_enable();
    while (1)
    {
        sleep_cpu();
        // just in case
        sei();
    }
}

