/*
 * timer.h
 *
 * Created: 01.04.2023 03:00:21
 *  Author: Bogusław
 */


#ifndef TIMER_H_
#define TIMER_H_

#include <avr/io.h>
#include <stdbool.h>

// maks 196 sekund
extern uint16_t __timer_3ms_counter;
// seven bits of counter, eight bit is permanent overflow flag
#define __timer_196s_counter GPIOR2

// setup 3ms event timer
extern void setup_timer_3ms();

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

// return true for the first pass of timed loop
static inline bool is_first_pass() {
    return  __timer_3ms_counter == 0 && __timer_196s_counter == 0;
}

#endif /* TIMER_H_ */