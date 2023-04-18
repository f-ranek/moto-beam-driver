/*
 * pwm.h
 *
 * Created: 27.03.2023 14:10:36
 *  Author: Bogusław
 */


#ifndef PWM_H_
#define PWM_H_

#include <avr/io.h>
#include <stdbool.h>

// setup PWM module
extern void start_pwm();

static inline void set_beam_pwm(uint8_t duty_cycle);

// ustawia diodę LED na włączoną (PWM) lub wyłączoną
static inline void set_led_on()
{
    // włączyć PWM
    // Table 11-6. Compare Output Mode, Fast PWM Mode
    // Clear OC0B on Compare Match, set OC0B at BOTTOM
    // (non-inverting mode)
    TCCR0A |= _BV(COM0B1);
}

static inline void set_led_off()
{
    // wartość pinu 0
    PORTA &= ~_BV(7);
    // odłączyć PWM
    TCCR0A &= ~_BV(COM0B1);
}


// zwraca true, gdy LED jest zaświecona
static inline bool is_led_on() {
    return TCCR0A & _BV(COM0B1);
}

// ustawia poziom mocy diody led
static inline void set_led_pwm(uint8_t duty_cycle) {
    OCR0B = duty_cycle;
}

// ustawia światła na włączone (100% mocy) lub wyłączone
static inline void set_beam_on_off(bool on)
{
    // ustawienie wartości portu
    if (on) {
        PORTB |= _BV(2);
     } else {
        PORTB &= ~_BV(2);
    }
    // odpięcie PWM
    TCCR0A &= ~_BV(COM0A1);
    set_beam_pwm(0);
}

// zmienia poziom mocy świateł
// wcześniej trzeba zawołać start_beam_pwm
static inline void set_beam_pwm(uint8_t duty_cycle) {
    OCR0A = duty_cycle;
}

// włącza światła i ustawia je na wybrany ułamek mocy
static inline void start_beam_pwm(uint8_t duty_cycle)
{
    set_beam_pwm(duty_cycle);
    // aktywacja wyjścia PWM
    // Table 11-3. Compare Output Mode, Fast PWM Mode
    // Clear OC0A on Compare Match
    // Set OC0A at BOTTOM (non-inverting mode)
    TCCR0A |= _BV(COM0A1);
}

static inline uint8_t get_beam_pwm_duty_cycle() {
    return OCR0A;
}

#endif /* PWM_H_ */