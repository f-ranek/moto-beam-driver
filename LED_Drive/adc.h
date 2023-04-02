/*
 * adc.h
 *
 * Created: 01.04.2023 02:29:09
 *  Author: Bogusław
 */


#ifndef ADC_H_
#define ADC_H_

#include <avr/io.h>

extern uint16_t __led_adc_result;
extern uint16_t __beam_adc_result;

// launch adc conversion for low beam value
static inline void launch_beam_adc()
{
    // start ADC - światła, PA3
    // REFS1 = 1 - 1.1 V as a reference
    // REFS1 = 0 - VCC as a reference
    // PA3 - MUX1, MUX0
    ADMUX = (0 << REFS1) | _BV(MUX1) | _BV(MUX0);
    // ADIE: ADC Interrupt Enable
    // ADEN: ADC Enable
    // ADPS0, ADPS1 - prescaler = 8, 125 kHz
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS0) | _BV(ADPS1);
    // ADC will start on CPU entering sleep state
}


// launch adc conversion for led value
#ifdef LED_PWM
static inline void launch_led_adc()
{
    // start ADC - LED, PA2
    // zgasić LED, port jako wejście
    DDRA &= ~_BV(7);
    // REFS1 = 1 - 1.1 V as a reference
    // REFS1 = 0 - VCC as a reference
    // PA2 - MUX1
    ADMUX = _BV(REFS1) | _BV(MUX1);
    // ADIE: ADC Interrupt Enable
    // ADEN: ADC Enable
    // ADPS0, ADPS1 - prescaler = 8, 125 kHz
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS0) | _BV(ADPS1);
    // ADC will start on CPU entering sleep state
}
#endif // LED_PWM

// get adc result from LED conversion
static inline uint16_t get_led_adc_result()
{
    return __led_adc_result;
}

// get adc result from lw beam conversion
static inline uint16_t get_beam_adc_result()
{
    return __beam_adc_result;
}

#endif /* ADC_H_ */