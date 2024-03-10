/*
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

extern uint8_t __accu_adc_status;
extern uint8_t __bulb_adc_status;

// launch adc conversion for low beam value
extern void launch_bulb_adc();
// launch adc conversion for accu voltage
extern void launch_accu_adc();

// get adc result from beam conversion
static inline bool is_bulb_adc_result_ready()
{
    return (__bulb_adc_status & 0x80) != 0;
}

// get adc result from beam conversion
static inline uint16_t get_bulb_adc_result()
{
    return __bulb_adc_result;
}

// get adc result from accu conversion
static inline bool is_accu_adc_result_ready()
{
    return (__accu_adc_status & 0x80) != 0;
}

// get adc result from accu conversion
static inline uint16_t get_accu_adc_result()
{
    return __accu_adc_result;
}

#endif /* ADC_H_ */