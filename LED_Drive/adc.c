/*
 * adc.c
 *
 * Created: 05.03.2023 16:42:56
 *  Author: Bogusław
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#include "functions.h"

//
uint16_t __led_adc_result;
uint16_t __low_beam_adc_result;

// adc done interrupt
ISR (ADC_vect, ISR_NAKED)
{
    // read ADCL
    // read ADCH
    uint16_t adc_result = ADC;
    if (bit_is_set(ADMUX, MUX0)) {
        // światła
        __low_beam_adc_result = adc_result;
     } else {
        // LED
        __led_adc_result = adc_result;
        // re-enable LED output
        DDRA |= _BV(DDA7); // ew. DDA5
    }

    // wyłączenie ADC
    ADCSRA = 0;

    reti();
}

// launch adc conversion for low beam value
inline void launch_low_beam_adc()
{
    // start ADC - światła, PA3
    // REFS1 = 1 - 1.1 V as a reference
    // REFS1 = 0 - VCC as a reference
    // PA3
    ADMUX = (0 << REFS1) | _BV(MUX1) | _BV(MUX0);
    // ADIE: ADC Interrupt Enable
    // ADEN: ADC Enable
    // ADPS0, ADPS1 - prescaler = 8, 125 kHz
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS0) | _BV(ADPS1);
    // ADC will start on CPU entering sleep state
}

// launch adc conversion for led value
inline void launch_led_adc()
{
    // start ADC - LED, PA2
    // zgasić LED, port jako wejście
    DDRA &= ~_BV(DDA7); // ew. DDA5
    // REFS1 = 1 - 1.1 V as a reference
    // REFS1 = 0 - VCC as a reference
    // PA2
    ADMUX = (0 << REFS1) | _BV(MUX1);
    // ADIE: ADC Interrupt Enable
    // ADEN: ADC Enable
    // ADPS0, ADPS1 - prescaler = 8, 125 kHz
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS0) | _BV(ADPS1);
    // ADC will start on CPU entering sleep state
}
