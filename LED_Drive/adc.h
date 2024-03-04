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

// get adc result from beam conversion
static inline uint16_t get_beam_adc_result()
{
    return __beam_adc_result;
}

#endif /* ADC_H_ */