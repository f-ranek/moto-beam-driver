﻿/*
 * adc.h
 *
 * Created: 01.04.2023 02:29:09
 *  Author: Bogusław
 */


#ifndef ADC_H_
#define ADC_H_

#include <stdbool.h>

extern uint16_t __accu_adc_result;
extern uint16_t __bulb_adc_result;
extern uint8_t __twilight_adc_result;

extern uint8_t __accu_adc_count;
extern uint8_t __bulb_adc_count;
extern uint8_t __twilight_adc_count;

extern uint16_t __adc_count;

// launch adc conversion for low beam value
extern void launch_bulb_adc();
// launch adc conversion for accu voltage
extern void launch_accu_adc();
// launch adc conversion for twilight sensor voltage
extern void launch_twilight_adc();

// is adc result from bulb sampling ready
static inline bool is_bulb_adc_result_ready()
{
    return (__bulb_adc_count & 0x80) != 0;
}

// get adc result from bulb conversion
extern uint16_t get_bulb_adc_result();

// is adc result from accu sampling ready
static inline bool is_accu_adc_result_ready()
{
    return (__accu_adc_count & 0x80) != 0;
}

// get adc result from accu conversion
extern uint16_t get_accu_adc_result();

// is adc result from accu sampling ready
static inline bool is_twilight_adc_result_ready()
{
    return (__twilight_adc_count & 0x80) != 0;
}

// get adc result from twilight sensor
extern uint8_t get_twilight_adc_result();

// get adc result from accu conversion
static inline uint16_t exchange_adc_count()
{
    uint16_t result = __adc_count;
    __adc_count = 0;
    return result;
}

#endif /* ADC_H_ */