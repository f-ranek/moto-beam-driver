/*
 * main.h
 *
 * Created: 01.04.2023 22:42:59
 *  Author: Bogusław
 */


#ifndef MAIN_H_
#define MAIN_H_

#include <avr/io.h>
#include <avr/pgmspace.h>

extern uint8_t restart_count;
#define MCUSR_initial_copy GPIOR0

extern const char VERSION[] PROGMEM;

#endif /* MAIN_H_ */