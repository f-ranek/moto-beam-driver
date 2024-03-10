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

static uint16_t process_adc_result(uint16_t* arr, uint16_t data, uint8_t* status, int8_t bit_size);

#define BULB1T_ADC_BITS 5
#define BULB2T_ADC_BITS 5
#define BULB3T_ADC_BITS 3
#define BULB_ADC_BITS  3
#define ACCU_ADC_BITS  1

#if (BULB1T_ADC_BITS>=7) || (BULB1T_ADC_BITS>=7) || (BULB3T_ADC_BITS>=7) || (BULB_ADC_BITS>=7) || (ACCU_ADC_BITS>=7)
#define sum_t uint32_t
#else
#define sum_t uint16_t
#endif

#define ADC_ARR_SIZE(bits) (2+(1<<(bits)))

#define ACCU_ADC_SIZE ADC_ARR_SIZE(ACCU_ADC_BITS)
#define BULB1_ADC_SIZE ADC_ARR_SIZE(BULB1T_ADC_BITS)
#define BULB2_ADC_SIZE ADC_ARR_SIZE(BULB2T_ADC_BITS)
#define BULB3_ADC_SIZE ADC_ARR_SIZE(BULB3T_ADC_BITS)
#define BULB_ADC_SIZE ADC_ARR_SIZE(BULB_ADC_BITS)

uint16_t __accu_adc_result;
uint16_t __bulb_adc_result;

uint16_t __accu_adc_results[ACCU_ADC_SIZE];
uint16_t __bulb1t_adc_results[BULB1_ADC_SIZE];
uint16_t __bulb2t_adc_results[BULB2_ADC_SIZE];
uint16_t __bulb3t_adc_results[BULB3_ADC_SIZE];
uint16_t __bulb_adc_results[BULB_ADC_SIZE];
uint8_t __accu_adc_status;
uint8_t __bulb1t_adc_status;
uint8_t __bulb2t_adc_status;
uint8_t __bulb3t_adc_status;
uint8_t __bulb_adc_status;

//uint8_t __adc_source;
//#define ADC_SOURCE_NONE 0
//#define ADC_SOURCE_ACCU 1
//#define ADC_SOURCE_BULB 2

#ifdef DEBUG
#define NO_RESULT 0x92F4
#else
#define NO_RESULT 0xFFFF
#endif // DEBUG

#ifndef SIMULATION
static inline
#endif // SIMULATION
void process_bulb_adc_result(uint16_t data_item) {
    uint16_t temp = process_adc_result(__bulb1t_adc_results, data_item, &__bulb1t_adc_status, BULB1T_ADC_BITS);
    if (__bulb1t_adc_status & 0x80) {
        temp = process_adc_result(__bulb2t_adc_results, temp, &__bulb2t_adc_status, BULB2T_ADC_BITS);
        if (__bulb2t_adc_status & 0x80) {
            temp = process_adc_result(__bulb3t_adc_results, temp, &__bulb3t_adc_status, BULB3T_ADC_BITS);
            if (__bulb3t_adc_status & 0x80) {
                __bulb_adc_result = process_adc_result(__bulb_adc_results, temp, &__bulb_adc_status, BULB_ADC_BITS);
            }
        }
    }
}

static inline void process_accu_adc_result(uint16_t data_item) {
    __accu_adc_result = process_adc_result(__accu_adc_results, data_item, &__accu_adc_status, ACCU_ADC_BITS);
    //__accu_adc_result = data_item;
    //__accu_adc_status = 0x80;
}

static uint16_t process_adc_result(uint16_t* arr, uint16_t data_item, uint8_t* status, int8_t bit_size)
{
    #define arr_size ADC_ARR_SIZE(bit_size)
    uint8_t status_val = *status;
    uint8_t insertion_index = status_val & 0x7F;
    arr[insertion_index] = data_item;
    if (++insertion_index == arr_size) {
        insertion_index = 0;
        status_val = 0x80;
    } else {
        status_val = (status_val & 0x80) | insertion_index;
    }
    *status = status_val;
    if  ((status_val & 0x80) == 0) {
        // póki co nie mamy odczytu
        return NO_RESULT;
    }

    // liczymy pseudo średnią
    uint16_t min = arr[0];
    uint16_t max = min;
    sum_t sum = min;
    uint8_t i = 1;
    for (; i<arr_size; i++) {
        uint16_t elem = arr[i];
        if (elem < min) {
            min = elem;
        }
        if (elem > max) {
            max = elem;
        }
        sum += elem;
    }

    // pomijam ekstrema
    sum -= min;
    sum -= max;

    // dzielenie z zaokrągleniem
    sum = sum >> (bit_size-1);
    sum++;
    sum = sum >> 1;
    return sum;
}

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


inline void launch_bulb_adc()
{
    // if ADC is in progress, just skip the call
    //if (__adc_source != ADC_SOURCE_NONE) {
    if (ADCSRA != 0) {
        return ;
    }
    // start ADC - światła, PA3
    // PA3 - MUX1, MUX0
    ADMUX = _BV(MUX1) | _BV(MUX0);
    // __adc_source = ADC_SOURCE_BULB;
    start_adc();
}



inline void launch_accu_adc()
{
    // if ADC is in progress, just skip the call
    // if (__adc_source != ADC_SOURCE_NONE) {
    if (ADCSRA != 0) {
        return ;
    }
    // PA2 - MUX1
    ADMUX = _BV(MUX1);
    // __adc_source = ADC_SOURCE_ACCU;
    start_adc();
}

// adc done interrupt
// actually we could simply read ADC result on next execution of main app loop
ISR (ADC_vect, ISR_NAKED)
{
    cli();

    // read ADCL
    // read ADCH
    uint16_t adc_result = ADC;
    // MUX0 differentiates channel on PIN 2 and 3
    //if (__adc_source == ADC_SOURCE_BULB) {
    if (bit_is_set(ADMUX, MUX0)) {
        // światła
        process_bulb_adc_result(adc_result);
    // } else if(__adc_source == ADC_SOURCE_ACCU) {
    } else {
        // akumulator
        process_accu_adc_result(adc_result);
    }

    // wyłączenie autostartu ADC
    // ADCSRA &= ~_BV(ADIE);
    // wyłączenie ADC
    ADCSRA = 0;
    ADMUX = 0;
    // __adc_source = ADC_SOURCE_NONE;

    // toggle pin
    PINA |= _BV(7);

    sei();
    reti();
}

