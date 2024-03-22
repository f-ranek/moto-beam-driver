/*
 * app_adc.c
 *
 * Created: 22.03.2024 13:29:31
 *  Author: Bogusław
 */

#include "app_adc.h"

uint16_t accu_adc_result;
uint16_t bulb_adc_result;

/*
// tu by się przydało takie wyliczenie, że z bieżących wartości
// napięcia i pwm wyliczamy jakiś współczynnik korekcji
// i on później jest uwzględniany
static uint8_t calc_target_pwm_value(uint16_t accu_adc_result)
{
    if (accu_adc_result < VOLTAGE_12_5_V) {
        // napięcie AKU mniejsze niż 12,5 V
        return 255;
    }
    return 503 - (35 * accu_adc_result) / 128;
}
*/


void adjust_target_pwm_value(uint8_t current,
    uint16_t accu_adc_result, uint16_t bulb_adc_result,
    pwm_consumer_t pm_consumer)
{
    if (accu_adc_result < VOLTAGE_12_5_V || bulb_adc_result > VOLTAGE_6_V) {
        // napięcie AKU mniejsze niż 12,5 V
        // lub napięcie żarówki > 6 V
        pm_consumer(255);
        return ;
    }
    uint16_t actual_bulb_voltage = accu_adc_result - bulb_adc_result;

    if (actual_bulb_voltage < VOLTAGE_13_1_V) {
        // mniej niż 13,1 V - idziemy do góry
        uint8_t diff = (VOLTAGE_13_1_V - actual_bulb_voltage) / 4;
        if (diff == 0) {
            diff = 1;
        } else if (diff > 10) {
            diff = 10;
        }
        uint16_t target = (uint16_t)current + (uint16_t)diff;
        if (target > 255) {
            pm_consumer(255);
        } else {
            pm_consumer(target);
        }
        return ;
    }

    const uint16_t max = VOLTAGE_13_1_V + VOLTAGE_0_05_V;
    if (actual_bulb_voltage > max) {
        uint8_t diff = (actual_bulb_voltage - max) / 4;
        if (diff == 0) {
            diff = 1;
        } else if (diff > 10) {
            diff = 10;
        }
        uint8_t target = current - diff;
        pm_consumer(target);
        return ;
    }

    // nic nie trzeba robić
}
