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
extern uint16_t __beam_adc_result;

// setup initial device status
extern inline void setup_initial_port_status();

// perform periodic application logic
extern inline void loop_application_logic();

// launch adc conversion for low beam value
extern inline void launch_beam_adc();

// launch adc conversion for led value
extern inline void launch_led_adc();

// setup adc conversionif needed
extern inline void setup_adc();

// setup 3ms event timer
extern inline void setup_timer_3ms();

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
static inline uint16_t get_beam_adc_result() {
    return __beam_adc_result;
}


#endif /* FUNCTIONS_H_ */