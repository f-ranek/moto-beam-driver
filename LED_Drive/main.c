/*
 * LED drive.c
 *
 * Created: 05.03.2023 15:21:50
 * Author : Bogusław
 */

// 1 MHz
#define F_CPU 100000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "functions.h"
#include "pwm.h"

int main(void)
{
    setup_initial_port_status();
    setup_timer_3ms();
    start_pwm();

    while (1)
    {
        sei();
        sleep_enable();
        sleep_cpu();
    }
}

