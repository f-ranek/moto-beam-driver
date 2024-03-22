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

// -------------------------------------------------- //

static inline uint8_t get_bulb_pwm_duty_cycle() {
    return OCR0A;
}

// zmienia poziom mocy świateł
// wcześniej trzeba zawołać start_bulb_pwm
static inline void adjust_bulb_power(uint8_t duty_cycle) {
    OCR0A = duty_cycle;
}

// włącza światła i ustawia je na wybrany ułamek mocy
static inline void start_bulb_pwm(uint8_t duty_cycle)
{
    // 1 - zgaszone
    PORTB |= _BV(2);
    adjust_bulb_power(duty_cycle);
    // aktywacja wyjścia PWM
    // Table 11-3. Compare Output Mode, Fast PWM Mode
    // Clear OC0A on Compare Match
    // Set OC0A at BOTTOM (non-inverting mode)
    TCCR0A |= _BV(COM0A1) | _BV(COM0A0);
}

// ustawia światła na włączone (100% mocy) lub wyłączone
static inline void set_bulb_on_off(bool on)
{
    // ustawienie wartości portu
    if (on) {
        // 0 - świeci
        PORTB &= ~_BV(2);
    } else {
        // 1 - zgaszone
        PORTB |= _BV(2);
    }
    // odpięcie PWM
    TCCR0A &= ~(_BV(COM0A1) | _BV(COM0A0));
    adjust_bulb_power(0);
}

// return value from 0 - bulb off, through pwm duty cycle to 255 - bulb on
static inline uint8_t get_bulb_power() {
    if (get_bulb_pwm_duty_cycle() == 0) {
        return (PORTB & _BV(2)) ? 0 : 0xFF;
    } else {
        return get_bulb_pwm_duty_cycle();
    }
}

// set bulb power
// duty_cycle: from 0 - bulb off, through pwm duty cycle to 255 - bulb on
static inline void set_bulb_power(uint8_t duty_cycle)
{
    if (duty_cycle == 0) {
        set_bulb_on_off(false);
    } else if (duty_cycle == 255) {
        set_bulb_on_off(true);
    } else {
        if (get_bulb_pwm_duty_cycle() == 0) {
            start_bulb_pwm(duty_cycle);
        } else {
            adjust_bulb_power(duty_cycle);
        }
    }
}

#endif /* PWM_H_ */