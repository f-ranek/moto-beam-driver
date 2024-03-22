/*
 * app_adc.h
 *
 * Created: 22.03.2024 13:29:54
 *  Author: Bogusław
 */


#ifndef APP_ADC_H_
#define APP_ADC_H_

#include <stdint.h>

extern uint16_t accu_adc_result;
extern uint16_t bulb_adc_result;

#define VOLTAGE_3_V    (0xCF)
#define VOLTAGE_6_V    (0x19D)
#define VOLTAGE_10_V   (0x2B0)
#define VOLTAGE_11_5_V (0x318)
#define VOLTAGE_12_5_V (0x35C)
#define VOLTAGE_13_V   (0x37F)
#define VOLTAGE_13_1_V (0x386)
#define VOLTAGE_13_2_V (0x38D)
#define VOLTAGE_13_5_V (0x3A1)

#define VOLTAGE_0_1_V  (0x7)
#define VOLTAGE_0_05_V (0x3)
#define VOLTAGE_0_5_V  (0x23)

extern void launch_adc();
extern void read_adc_results();

typedef void (*pwm_consumer_t)(uint8_t);

extern void adjust_target_pwm_value(
    uint8_t current,
    uint16_t accu_adc_result, uint16_t bulb_adc_result,
    pwm_consumer_t pm_consumer);

static inline void adjust_target_pwm_value_2(
    uint8_t current,
    pwm_consumer_t pm_consumer)
{
    adjust_target_pwm_value(current, accu_adc_result, bulb_adc_result, pm_consumer);
}

typedef enum __accu_status {
    UNKNOWN,
    // rozrusznik
    STARTER_RUNNING,
    // brak ładowania
    NORMAL,
    // ładowanie
    CHARGING
} accu_status_e;

static inline accu_status_e get_accu_status()
{
    if (accu_adc_result < VOLTAGE_6_V) {
        return UNKNOWN;
    }
    if (accu_adc_result < VOLTAGE_10_V) {
        return STARTER_RUNNING;
    }
    if (accu_adc_result < VOLTAGE_13_V) {
        return NORMAL;
    }
    return CHARGING;
}

typedef enum __bulb_actual_status {
    // blisko zero V, przepalona żarówka lub bezpiczenik
    BULB_VOLTAGE_ZERO,

    // do 3V, normalna praca
    BULB_VOLTAGE_UNDER_LOAD,

    // powyżej 6 V, wszystko ok, żarówka wyłączona
    BULB_VOLTAGE_FULL,

    BULB_VOLTAGE_UNKNOWN_READING
} bulb_actual_status_e;

static inline bulb_actual_status_e get_bulb_actual_status()
{
    if (bulb_adc_result < VOLTAGE_0_1_V) {
        return BULB_VOLTAGE_ZERO;
    }
    if (bulb_adc_result < VOLTAGE_3_V) {
        return BULB_VOLTAGE_UNDER_LOAD;
    }
    if (bulb_adc_result > VOLTAGE_6_V) {
        return BULB_VOLTAGE_FULL;
    }
    return BULB_VOLTAGE_UNKNOWN_READING;
}

#endif /* APP_ADC_H_ */