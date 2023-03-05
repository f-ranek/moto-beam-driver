/*
 * functions.h
 *
 * Created: 05.03.2023 16:36:29
 *  Author: Bogusław
 */


#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

extern uint16_t __timer_3ms_counter;

extern uint16_t __led_adc_result;
extern uint16_t __low_beam_adc_result;

// setup initial device status
extern inline void setup_initial_port_status();

// perform periodic application logic
extern inline void loop_application_logic();

// read stable pin values
extern inline void read_pin_values();

// interpret ADC readings into logical states
extern inline void calculate_adc_readings();

// execute application logic
extern inline void execute_state_transition_changes();

// update pwm outputs according to application status
extern inline void adjust_pwm_values();

// launch adc conversion for low beam value
extern inline void launch_low_beam_adc();

// launch adc conversion for led value
extern inline void launch_led_adc();

// setup adc conversionif needed
extern inline void setup_adc();

// setup 3ms event timer
extern inline void setup_timer_3ms();

// get current timer value
static inline uint16_t get_timer_value() {
    return __timer_3ms_counter;
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