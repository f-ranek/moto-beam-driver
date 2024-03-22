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

#define VOLTAGE_10_V   (0x2B0)
#define VOLTAGE_11_5_V (0x318)
#define VOLTAGE_12_5_V (0x35C)
#define VOLTAGE_13_V   (0x37F)
#define VOLTAGE_13_1_V (0x386)
#define VOLTAGE_13_2_V (0x38D)
#define VOLTAGE_13_5_V (0x3A1)


#define VOLTAGE_6_V    (0x1A0)
#define VOLTAGE_0_1_V  (0x7)
#define VOLTAGE_0_05_V (0x3)
#define VOLTAGE_0_5_V  (0x23)

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

#endif /* APP_ADC_H_ */