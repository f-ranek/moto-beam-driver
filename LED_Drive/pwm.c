/*
 * pwm.c
 *
 * Created: 05.03.2023 22:41:57
 *  Author: Bogusław
 */

#include <avr/io.h>
#include "pwm.h"

// LED
// OCR0B, PA7

// światła
// OCR0A, PB2

// setup pwm module
void start_pwm()
{

    // Table 11-8. Waveform Generation Mode Bit Description
    // Fast PWM from 0 to 255
    // ew. Phase Correct PWM, wtedy częstotliwość jest 2 razy mniejsza...
    TCCR0A = _BV(WGM01) | _BV(WGM00);

    // Table 11-9. Clock Select Bit Description
    // 001 - no prescaler
    // 010 - /8
    // 011 - /64
    // 100 - /256
    // 101 - /1024
    // fOCnxPWM = fclk_I/O / (N * 256), where N = prescaler factor of 1, 8, 64, 256, 1024

    // from 4 MHz and 256 prescaler, we get ~61 Hz
    // TCCR0B = _BV(CS02);

    // from 4 MHz and 64 prescaler, we get ~244 Hz
    TCCR0B = _BV(CS01) | _BV(CS00);

    // fig 11-6 pg 76
    //COM0x1:0 = 2 - non inverted pwm
    //COM0x1:0 = 3 - inverted pwm

    // port LED jako wyjście
    PORTA &= ~_BV(7);
    DDRA |= _BV(7);

    // port świateł jako wyjście
    PORTB |= _BV(2);
    DDRB |= _BV(2);

    // default LED duty cycle - 5%
    OCR0B = 12;
}

