/*
 * adc.h
 *
 * Created: 01.04.2023 02:29:09
 *  Author: Bogusław
 */


#ifndef ADC_H_
#define ADC_H_

#include <stdbool.h>

extern uint16_t __led_adc_result;
extern uint16_t __beam_adc_result;

// launch adc conversion for low beam value
extern void launch_beam_adc();

// launch adc conversion for led value
#ifdef LED_ADC

extern uint16_t __led_adc_accumulator;
extern uint8_t __led_adc_counter;

extern void launch_led_adc();

// get adc result from LED conversion
static inline uint16_t get_led_adc_result()
{
    return __led_adc_result;
}

// return true if new conversion result is available, then
// reset availability flag
static inline bool exchange_led_adc_result_available()
{
    bool available = __led_adc_counter == 8;
    if (available) {
        __led_adc_counter = 0;
    }
    return available;
}

#endif // LED_ADC

// get adc result from beam conversion
static inline uint16_t get_beam_adc_result()
{
    return __beam_adc_result;
}

#endif /* ADC_H_ */