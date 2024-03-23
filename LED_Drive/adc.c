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

// PA2 - napięcie AKU
// PA3 - światła

uint16_t __accu_adc_sum;
uint8_t __accu_adc_count;
const uint8_t __accu_adc_bits = 5;

uint16_t __bulb_adc_sum;
uint8_t __bulb_adc_count;
const uint8_t __bulb_adc_target = 40;

uint8_t __adc_source;
#define ADC_SOURCE_NONE 0
#define ADC_SOURCE_ACCU 1
#define ADC_SOURCE_BULB 2

uint16_t __adc_count;

#ifdef DEBUG

uint16_t __dbg_bulb_adc_count;
uint16_t __dbg_accu_adc_count;

#define INCREMENT_IF_DEBUG(i) do{ i++; }while(0)

#define NO_RESULT 0x92F4
#define return_if_not_ready(adc_count)  \
do {                                    \
    if ((adc_count & 0x80) == 0) {      \
        return NO_RESULT;               \
    }                                   \
} while (0)


#else

#define INCREMENT_IF_DEBUG(i) do{ }while(0)

#define return_if_not_ready(adc_count) do { } while (0)

#endif // DEBUG


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
    ADCSRA = _BV(ADEN) | _BV(ADIE)  | _BV(ADIF) | _BV(ADSC) | _BV(ADATE) | _BV(ADPS2) | _BV(ADPS0);
}

static inline void stop_adc()
{
    ADCSRA = _BV(ADEN) | _BV(ADIF) | _BV(ADPS2) | _BV(ADPS0);
    __adc_source = ADC_SOURCE_NONE;
}

inline void launch_bulb_adc()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        __adc_source = 0x20 | ADC_SOURCE_BULB;
        // reset, aby zachować ciągłość pomiaru
        __bulb_adc_sum = 0;
        __bulb_adc_count = 0;
    }
    // start ADC - światła, PA3
    // PA3 - MUX1, MUX0
    ADMUX = _BV(MUX1) | _BV(MUX0);
    start_adc();
}

inline void launch_accu_adc()
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        __adc_source = 0x20 | ADC_SOURCE_ACCU;
        // reset, aby zachować ciągłość pomiaru
        __accu_adc_sum = 0;
        __accu_adc_count = 0;
    }
    // PA2 - MUX1
    ADMUX = _BV(MUX1);
    start_adc();
}

static inline void process_bulb_adc_result(uint16_t data_item) {
    // 244 Hz PWM dają cykl około 4,1 ms
    // co przy 13 cyklach na konwersję przy 125 kHz
    // daje 0,104 ms potrzebne na próbkowanie sygnału
    // co daje 39 próbek, które trzeba zebrać, żeby uśrednić sygnał

    __bulb_adc_sum += data_item;
    if (++__bulb_adc_count == __bulb_adc_target) {
        stop_adc();
        __bulb_adc_count = 0xFF;
        INCREMENT_IF_DEBUG(__dbg_bulb_adc_count);
    }
}

uint16_t get_bulb_adc_result()
{
    return_if_not_ready(__bulb_adc_count);
    // calc mean of 40 results
    uint16_t result = __bulb_adc_sum;
    result /= 20;
    result ++;
    result >>= 1;
    return result;
}

static inline void process_accu_adc_result(uint16_t data_item) {
    __accu_adc_sum += data_item;
    if (++__accu_adc_count == _BV(__accu_adc_bits)) {
        stop_adc();
        __accu_adc_count = 0xFF;
        INCREMENT_IF_DEBUG(__dbg_accu_adc_count);
    }
}

uint16_t get_accu_adc_result()
{
    return_if_not_ready(__accu_adc_count);
    uint16_t result = __accu_adc_sum;
    result >>= __accu_adc_bits - 1;
    result ++;
    result >>= 1;
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
        // read ADCL
        // read ADCH
        uint16_t adc_result = ADC;
        process_bulb_adc_result(adc_result);
    } else  {
        __adc_source -= 0x10;
    }

    // wdt_reset(); // TEMP
}
