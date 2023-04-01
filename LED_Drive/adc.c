﻿/*
 * adc.c
 *
 * Created: 05.03.2023 16:42:56
 *  Author: Bogusław
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#include "adc.h"

// PA2 - LED in
// PA3 - światła
// PA7 - LED out

uint16_t __led_adc_result;
uint16_t __beam_adc_result;

// adc done interrupt
ISR (ADC_vect, ISR_NAKED)
{
    // read ADCL
    // read ADCH
    uint16_t adc_result = ADC;
    if (bit_is_set(ADMUX, MUX0)) {
        // światła
        __beam_adc_result = adc_result;
     } else {
        // LED
        __led_adc_result = adc_result;
        // re-enable LED output
        DDRA |= _BV(7); // ew. 5
    }

    // wyłączenie ADC
    ADCSRA = 0;

    reti();
}

