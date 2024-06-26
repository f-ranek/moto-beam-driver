﻿/*
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

// get current timer value - last 8 bits
static inline uint8_t get_timer8_value() {
    // little endian - mico optimization
    return *((uint8_t*)(&__timer_3ms_counter));
}

// 1 second in 3ms ticks
#define INTERVAL_ONE_SECOND ((uint16_t)(1000 / 3))
// 2 seconds in 3ms ticks
#define INTERVAL_TWO_SECONDS ((uint16_t)(2000 / 3))
// 3 seconds in 3ms ticks
#define INTERVAL_THREE_SECONDS ((uint16_t)(3000 / 3))
// 5 seconds in 3ms ticks
#define INTERVAL_FIVE_SECONDS ((uint16_t)(5000 / 3))

// 1/5 second in 3ms ticks
#define INTERVAL_FIFTH_SECOND ((uint16_t)(200 / 3))
#define INTERVAL_FOUR_FIFTH_SECOND ((uint16_t)(INTERVAL_ONE_SECOND - INTERVAL_FIFTH_SECOND))
#define INTERVAL_HALF_A_SECOND ((uint16_t)(500 / 3))

#define INTERVAL_1_20_SECOND ((uint16_t)(50 / 3))
#define INTERVAL_19_20_SECOND ((uint16_t)(INTERVAL_ONE_SECOND - INTERVAL_1_20_SECOND))

#endif /* TIMER_H_ */