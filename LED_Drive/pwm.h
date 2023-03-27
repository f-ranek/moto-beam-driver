/*
 * pwm.h
 *
 * Created: 27.03.2023 14:10:36
 *  Author: Bogusław
 */


#ifndef PWM_H_
#define PWM_H_

// setup PWM module
extern void start_pwm();

// ustawia diodę LED na włączoną (PWM) lub wyłączoną
extern void set_led(uint8_t on);

// ustawia diodę led na włączoną na zadany okres cyklu
extern void set_led_pwm(uint8_t duty_cycle);

// ustawia światła na włączone (100% mocy) lub wyłączone
extern void set_beam_on_off(uint8_t on);

// ustawia światła na wybrany ułamek mocy
extern void start_beam_pwm(uint8_t duty_cycle);

// ustawia światła na wybrany ułamek mocy
// wcześniej trzeba zawołać start_beam_pwm
static inline void set_beam_pwm(uint8_t duty_cycle) {
    OCR0A = duty_cycle;
}

#define get_beam_pwm_duty_cycle() (OCR0A)



#endif /* PWM_H_ */