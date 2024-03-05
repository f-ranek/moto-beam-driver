/*
 * adc.h
 *
 * Created: 01.04.2023 02:29:09
 *  Author: Bogusław
 */


#ifndef ADC_H_
#define ADC_H_

#include <stdbool.h>

extern uint16_t __power_adc_result;
extern uint16_t __bulb_adc_result;

// launch adc conversion for low beam value
extern void launch_bulb_adc();

// get adc result from beam conversion
static inline uint16_t get_bulb_adc_result()
{
    return __bulb_adc_result;
}

#endif /* ADC_H_ */