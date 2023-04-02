/*
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
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS1) | _BV(ADPS2) | _BV(ADPS0);
    // ADC will start on CPU entering sleep state
}


inline void launch_beam_adc()
{
    // start ADC - światła, PA3
    // REFS1 = 1 - 1.1 V as a reference
    // REFS1 = 0 - VCC as a reference
    // PA3 - MUX1, MUX0
    ADMUX = (0 << REFS1) | _BV(MUX1) | _BV(MUX0);
    start_adc();
}

#ifdef LED_ADC

uint16_t __led_adc_accumulator;
uint8_t __led_adc_counter;

inline void launch_led_adc()
{
    // może przed konwersją powinniśmy poczekać jakiś jeden cykl
    // puścić ADC np w trybie free running
    // pierwsze cztery konwersje zignorować
    // a następnie uśrednić z 16 wyników...?

    // start ADC - LED, PA2
    // port jako wejście - to odpina PWM
    DDRA &= ~_BV(7);
    // na wszelki wypadek- ustawienie 0 - wyłączenie internal pull-up resistor
    PORTA &= ~_BV(7);

    __led_adc_accumulator = 0;
    __led_adc_counter = 0;

    // REFS1 = 1 - 1.1 V as a reference
    // REFS1 = 0 - VCC as a reference
    // PA2 - MUX1
    ADMUX = _BV(REFS1) | _BV(MUX1);
    start_adc();
}

inline static void process_led_adc_results()
{
    // first conversion takes 25 cycles
    // next one takes 13 cycles, let's assume 15
    // we have ADC clock running at 125 kHz
    // this means during 3 ms period we have 375 conversion cycles available
    // this means we have 25 readings possible, not taking into account application code
    // first one taking 0,2 ms
    // and next one taking 0,12 ms

    // so execute 4 conversions, discard results
    // then execute next 4 conversion, and avg results or maybe better calc median?
    // then wait until we are invoked again

    uint8_t counter = ++__led_adc_counter;
    if (counter == 8) {
        // got it
        // re-enable LED output
        DDRA |= _BV(7);
        // disable ADC
        ADCSRA = 0;
        // calc result
        uint16_t adc_result = ADC;
        __led_adc_result = (__led_adc_accumulator + adc_result) / 4;
        // just to be on the safe side
        __led_adc_accumulator = 0;
    } else if (counter > 4) {
        uint16_t adc_result = ADC;
        __led_adc_accumulator += adc_result;
    }
}

#endif // LED_ADC

// adc done interrupt
// actually we could simple read ADC result on next execution of main app loop
ISR (ADC_vect, ISR_NAKED)
{
    // read ADCL
    // read ADCH
    uint16_t adc_result = ADC;
    #ifdef LED_ADC
    // MUX0 differentiates channel on PIN 2 and 3
    if (bit_is_set(ADMUX, MUX0)) {
        // światła
        __beam_adc_result = adc_result;
        // wyłączenie ADC
        ADCSRA = 0;
    } else {
        // LED
        process_led_adc_results();
    }
    #else // LED_ADC
    __beam_adc_result = adc_result;
    // wyłączenie ADC
    ADCSRA = 0;
    #endif // !LED_ADC

    reti();
}

