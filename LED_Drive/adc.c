/*
 * adc.c
 *
 * Created: 05.03.2023 16:42:56
 *  Author: Bogusław
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>


#include "adc.h"

// PA2 - światła
// PA3 - napięcie AKU

uint16_t __accu_adc_sum;
uint8_t __accu_adc_count;

// czas do wyniku: 7 ms
const uint8_t __accu_adc_target = 64;

__uint24 __bulb_adc_sum;
uint8_t __bulb_adc_count;

// 125 Hz PWM dają cykl 8 ms
// co przy 13 cyklach na konwersję przy 125 kHz
// daje 0,104 ms potrzebne na próbkowanie sygnału
// co daje 78 próbek, które trzeba zebrać, żeby uśrednić sygnał

// czas do wyniku: 8,5 ms
const uint8_t __bulb_adc_target = 78;

uint16_t __twilight_adc_sum;
uint8_t __twilight_adc_count;

// czas do wyniku: 1,2 ms
const uint8_t __twilight_adc_target = 8;

uint8_t __adc_source;

// liczba pierwszych próbek, które ignorujemy
#define ADC_CONV_DELAY 3

#define map_adc_source(src) (((ADC_CONV_DELAY) << 4) | (src))

uint16_t __adc_count;

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
    // for 4MHz clock, we need to /32 to get 125 kHz
    // for 8MHz clock, we need to /64 to get 125 kHz
    // for 8MHz clock, we need to /128 to get 62,5 kHz
    ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADIF) | _BV(ADSC) | _BV(ADATE) | _BV(ADPS2) | _BV(ADPS1);
}

static inline void stop_adc()
{
    // ADCSRA = _BV(ADEN) | _BV(ADIF) | _BV(ADPS2) | _BV(ADPS1);
    __adc_source = ADC_SOURCE_NONE;
}

inline void launch_bulb_adc()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        __adc_source = map_adc_source(ADC_SOURCE_BULB);
        // reset, aby zachować ciągłość pomiaru
        __bulb_adc_sum = 0;
        __bulb_adc_count = 0;
    }
    // start ADC - światła
    // PA3 - MUX1, MUX0
    ADMUX = _BV(MUX1) | _BV(MUX0);
    start_adc();
}

inline void launch_accu_adc()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        __adc_source = map_adc_source(ADC_SOURCE_ACCU);
        // reset, aby zachować ciągłość pomiaru
        __accu_adc_sum = 0;
        __accu_adc_count = 0;
    }
    // PA2 - MUX1
    ADMUX = _BV(MUX1);
    start_adc();
}

inline void launch_twilight_adc()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        __adc_source = map_adc_source(ADC_SOURCE_TWILIGHT);
        // reset, aby zachować ciągłość pomiaru
        __twilight_adc_sum = 0;
        __twilight_adc_count = 0;
    }
    // PA1 - MUX0
    ADMUX = _BV(MUX0);
    start_adc();
}

static inline void process_adc_result_16(
    uint16_t* accumulator, uint16_t data_item,
    uint8_t* counter, uint8_t target_count)
{
    *accumulator += data_item;
    if (++ (*counter) == target_count) {
        stop_adc();
        *counter = 0xFF;
        __adc_source = 0;
    }
}

static inline void process_adc_result_24(
    __uint24* accumulator, uint16_t data_item,
    uint8_t* counter, uint8_t target_count)
{
    *accumulator += data_item;
    if (++ (*counter) == target_count) {
        stop_adc();
        *counter = 0xFF;
        __adc_source = 0;
    }
}

static inline uint16_t get_adc_result_16(uint16_t accumulator, uint8_t target_count)
{
    uint16_t result = accumulator / (target_count / 2);
    result ++;
    result >>= 1;
    return result;
}

static inline uint16_t get_adc_result_24(__uint24 accumulator, uint8_t target_count)
{
    uint16_t result = accumulator / (target_count / 2);
    result ++;
    result >>= 1;
    return result;
}

static inline void process_bulb_adc_result(uint16_t data_item)
{
    process_adc_result_24(&__bulb_adc_sum, data_item, &__bulb_adc_count, __bulb_adc_target);
}

uint16_t get_bulb_adc_result()
{
    // calc mean of 78 results
    return get_adc_result_24(__bulb_adc_sum, __bulb_adc_target);
}

static inline void process_accu_adc_result(uint16_t data_item)
{
    process_adc_result_16(&__accu_adc_sum, data_item, &__accu_adc_count, __accu_adc_target);
}

uint16_t get_accu_adc_result()
{
    return get_adc_result_16(__accu_adc_sum, __accu_adc_target);
}

static inline void process_twilight_adc_result(uint16_t data_item)
{
    process_adc_result_16(&__twilight_adc_sum, data_item, &__twilight_adc_count, __twilight_adc_target);
}

uint8_t get_twilight_adc_result()
{
    uint16_t result = get_adc_result_16(__twilight_adc_sum, __twilight_adc_target);
    // z 12 bit schodzę na 8
    result /= 4;
    return result;
}

// adc done interrupt
ISR (ADC_vect, ISR_BLOCK)
{
    __adc_count++;
    if(__adc_source == ADC_SOURCE_ACCU) {
        // akumulator
        uint16_t adc_result = ADC;
        process_accu_adc_result(adc_result);
    } else if (__adc_source == ADC_SOURCE_BULB) {
        // światła
        uint16_t adc_result = ADC;
        process_bulb_adc_result(adc_result);
    } else if (__adc_source == ADC_SOURCE_TWILIGHT) {
        // detektor zmierzchu
        uint16_t adc_result = ADC;
        process_twilight_adc_result(adc_result);
    } else  {
        __adc_source -= 0x10;
    }
}
