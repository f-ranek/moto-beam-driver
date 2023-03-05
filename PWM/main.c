/*
 * PWM.c
 *
 * Created: 28.02.2023 22:17:54
 * Author : Bogusław
 */

// 1 MHz
#define F_CPU 100000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

int main(void)
{
    // Toggle OC0A on Compare Match
    // TCCR0A = _BV(COM0A0)

    // DDR

    // Fast PWM Mode

    DDRA |= _BV(DDA7);
    DDRB |= _BV(DDB2);

    // toggle count value - duty cycle - PA7
    OCR0B = 200;
    // toggle count value - duty cycle - PB2
    OCR0A = 25; // tym ew. można skrócić cykl, jeżeli ustawimy w TCCR0B bit WGM02, ale wtedy trzeba odciąć pin PB2

    //
    TCCR0A = _BV(WGM01) | _BV(WGM00) | _BV(COM0A1) | _BV(COM0B1);

    // Table 11-9. Clock Select Bit Description
    //TCCR0B = _BV(CS02);
    TCCR0B = _BV(CS00);
    // WGM02:0 =  7

    // fig 11-6 pg 76
    //COM0x1:0 = 2 - non inverted pwm
    //COM0x1:0 = 3 - inverted pwm

    // fOCnxPWM = fclk_I/O / (N * 256), where N = prescaler factor of 1, 8, 64, 256, 1024

    sei();
    sleep_enable();
    while (1)
    {
        sleep_cpu();
        // just in case
        sei();
    }
}

