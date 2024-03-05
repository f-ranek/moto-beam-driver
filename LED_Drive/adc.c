/*
 * adc.c
 *
 * Created: 05.03.2023 16:42:56
 *  Author: Bogusław
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#include "adc.h"

// PA2 - napięcie AKU
// PA3 - światła

uint16_t __beam_adc_result;

static inline void start_adc()
{
    // ADIE: ADC Interrupt Enable
    // ADEN: ADC Enable
    // Table 16-6. ADC Prescaler Selections
    // 000 - /2
    // 001 - /2
    // 010 - /4
    // 011 - /8
    // 100 - /16
    // 101 - /32
    // 110 - /64
    // 111 - /128
    // target frequency should be in range 50 - 200 kHz
    // so for 4MHz clock, we need to /32 to get 125 kHz
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS0);
    // ADC will start on CPU entering sleep state

}


inline void launch_beam_adc()
{
    // if ADC is in progress, just skip the call
    if (ADCSRA & _BV(ADSC)) {
        return ;
    }

    __beam_adc_result = 0xFFFF;

    // start ADC - światła, PA3
    // REFS1 = 1 - 1.1 V as a reference
    // REFS1 = 0 - VCC as a reference
    // PA3 - MUX1, MUX0
    ADMUX = _BV(MUX1) | _BV(MUX0);
    start_adc();
}


uint16_t __power_adc_result;

inline void launch_power_adc()
{
    // if ADC is in progress, just skip the call
    if (ADCSRA & _BV(ADSC)) {
        return ;
    }

    __power_adc_result = 0xFFFF;

    // REFS1 = 1 - 1.1 V as a reference
    // REFS1 = 0 - VCC as a reference
    // PA2 - MUX1
    ADMUX = _BV(MUX1);
    start_adc();
}

// adc done interrupt
// actually we could simply read ADC result on next execution of main app loop
ISR (ADC_vect, ISR_NAKED)
{
    // read ADCL
    // read ADCH
    uint16_t adc_result = ADC;
    // MUX0 differentiates channel on PIN 2 and 3
    if (bit_is_set(ADMUX, MUX0)) {
        // światła
        __beam_adc_result = adc_result;
    } else {
        // LED
        __power_adc_result = adc_result;
    }

    // wyłączenie autostartu ADC
    ADCSRA &= ~_BV(ADIE);

    reti();
}

