/*
 * functions.h
 *
 * Created: 05.03.2023 16:36:29
 *  Author: Bogusław
 */


#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

// maks 196 sekund
extern uint16_t __timer_3ms_counter;
// seven bits of counter, eight bit is permanent overflow flag
extern uint8_t __timer_196s_counter;

extern uint16_t __led_adc_result;
extern uint16_t __low_beam_adc_result;

// setup initial device status
extern inline void setup_initial_port_status();

// perform periodic application logic
extern inline void loop_application_logic();

// read stable pin values
extern inline void read_pin_values();

// launch adc conversion for low beam value
extern inline void launch_low_beam_adc();

// launch adc conversion for led value
extern inline void launch_led_adc();

// setup adc conversionif needed
extern inline void setup_adc();

// setup 3ms event timer
extern inline void setup_timer_3ms();

// setup PWM module
extern void start_pwm();

// ustawia diodę LED na włączoną (PWM) lub wyłączoną
extern void set_led(uint8_t on);

// ustawia diodę led na włączoną na zadany okres cyklu
extern void set_led_pwm(uint8_t duty_cycle);

// ustawia światła na włączone lub wyłączone
extern void set_low_beam_on_off(uint8_t on);

// ustawia światła na wybrany ułamek mocy
extern void set_low_beam_pwm(uint8_t duty_cycle);

// get current timer value
// incremented every 3 ms
static inline uint16_t get_timer_value() {
    return __timer_3ms_counter;
}

// get current timer 2 value
// incremented every ~196 seconds
static inline uint8_t get_timer_2_value() {
    return __timer_196s_counter & ~_BV(7);
}

// return true for the first 196 seconds since device start
static inline uint8_t is_first_pass() {
    return __timer_196s_counter == 0;
}

// get adc result from LED conversion
static inline uint16_t get_led_adc_result() {
    return __led_adc_result;
}

// get adc result from lw beam conversion
static inline uint16_t get_low_beam_adc_result() {
    return __low_beam_adc_result;
}

typedef struct __pin_status_s {
    uint8_t temp_status_since;
    uint8_t curr_status;
    uint8_t temp_status;
} __pin_status;

extern __pin_status __ignition_starter_status;
extern __pin_status __neutral_status;
extern uint8_t __button_interrupt_pending;

// zwraca dwa bity opisujące stan wejść zapłonu oraz rozrusznika
static inline uint8_t get_ignition_starter_status() {
    return __ignition_starter_status.curr_status;
}

// zwraca jeden bit opisujący stan wejścia biegu neutralnego
static inline uint8_t get_neutral_status() {
    return __neutral_status.curr_status;
}

// zwraca oraz resetuje flagę informującą o zwolnieniu przycisku
static inline uint8_t exchange_button_release_flag() {
    uint8_t result = __button_interrupt_pending;
    __button_interrupt_pending = 0;
    return result;
}

#endif /* FUNCTIONS_H_ */