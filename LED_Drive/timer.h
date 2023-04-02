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

// setup 3ms event timer
extern void setup_timer_3ms();

// get current timer value
// incremented every 3 ms
static inline uint16_t get_timer_value() {
    return __timer_3ms_counter;
}

#endif /* TIMER_H_ */