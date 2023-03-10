/*
 * pwm.c
 *
 * Created: 05.03.2023 22:41:57
 *  Author: Bogusław
 */

#include <avr/io.h>

// LED
// OCR0B, PA7
// ew:
// OCR1B, PA5, ale wtedy trzeba skalować do 255 do 374

// światła
// OCR0A, PB2

// setup pwm module
void start_pwm()
{
    // Table 11-8. Waveform Generation Mode Bit Description
    // Fast PWM from 0 to 255
    // ew. Phase Correct PWM, wtedy częstotliwość jet 2 razy mniejsza...
    // zweryfikować, jak wtedy działają bity COM0A1 i COM0A0
    TCCR0A = _BV(WGM01) | _BV(WGM00);

    // Table 11-9. Clock Select Bit Description
    TCCR0B = _BV(CS02); // /256 -> 15 Hz - mało
    // TCCR0B = _BV(CS00) | _BV(CS00); // /8 -> 61 Hz - chyba ok

    // fig 11-6 pg 76
    //COM0x1:0 = 2 - non inverted pwm
    //COM0x1:0 = 3 - inverted pwm

    // fOCnxPWM = fclk_I/O / (N * 256), where N = prescaler factor of 1, 8, 64, 256, 1024

    // port LED jako wyjście
    PORTA &= ~_BV(7); // ew. 5
    DDRA |= _BV(7); // ew. 5

    // port świateł jako wyjście
    PORTB &= ~_BV(2);
    DDRB |= _BV(2);
}

#define disable_led_pwm() do { TCCR0A &= ~_BV(COM0B1); } while (0)
#define enable_led_pwm() do { TCCR0A |= _BV(COM0B1); } while (0)

// ustawia diodę LED na włączoną (PWM) lub wyłączoną
void set_led(uint8_t on)
{
    if (on == 0) {
        // wartość pinu 0
        PORTA &= ~_BV(7); // ew. 5
        // odłączyć PWM
        disable_led_pwm();
    } else {
        // włączyć PWM
        enable_led_pwm();
    }
}

// ustawia diodę led na włączoną na zadany okres cyklu
void set_led_pwm(uint8_t duty_cycle)
{
    if (duty_cycle == 255) {
        // wartość pinu 0
        PORTA |= _BV(7); // ew. 5
        // odłączyć PWM
        disable_led_pwm();
    } else {
        OCR0B = duty_cycle;
        // podpięcie pod PA7
        enable_led_pwm();
    }
}

#define disable_beam_pwm() do { TCCR0A &= ~_BV(COM0A1); } while (0)
// Table 11-3. Compare Output Mode, Fast PWM Mode
// Clear OC0A on Compare Match
// Set OC0A at BOTTOM (non-inverting mode)
#define enable_beam_pwm() do { TCCR0A |= _BV(COM0A1); } while (0)

// ustawia światła na włączone lub wyłączone
void set_low_beam_on_off(uint8_t on)
{
    // ustawienie wartości portu
    if (on) {
        PORTB |= _BV(2);
    } else {
        PORTB &= ~_BV(2);
    }
    // odpięcie PWM
    disable_beam_pwm();
}

// ustawia światła na wybrany ułamek mocy
void set_low_beam_pwm(uint8_t duty_cycle)
{
    OCR0A = duty_cycle;
    // aktywacja wyjścia PWM
    enable_beam_pwm();
} 