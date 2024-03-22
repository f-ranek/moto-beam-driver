/*
 * app_adc.c
 *
 * Created: 22.03.2024 13:29:31
 *  Author: Bogusław
 */

#include "app_adc.h"
#include "adc.h"
#include "application.h"
#include "timer.h"

uint16_t accu_adc_result;
uint16_t bulb_adc_result;

static inline __attribute__ ((always_inline)) uint16_t reverse_bytes(uint16_t arg) {
    typedef uint8_t v2 __attribute__((vector_size (2)));
    v2 vec;
    vec[0] = arg >> 8;
    vec[1] = arg;
    return (uint16_t)vec;
}

static inline uint16_t smooth_value(uint16_t old_val, uint16_t new_val)
{
    if (old_val == 0) {
        return new_val;
        } else {
        return (new_val + 3 * old_val) / 4;
    }
}

static uint16_t next_adc_counter_read_timeline;
void read_adc_results()
{
    if (is_bulb_adc_result_ready()) {
        bulb_adc_result = smooth_value(bulb_adc_result, get_bulb_adc_result());
        app_debug_status.bulb_voltage_lo = bulb_adc_result;
        app_debug_status.adc_voltage_hi = (app_debug_status.adc_voltage_hi & 0x0F) | ((reverse_bytes(bulb_adc_result) & 0x0F) << 4);
    }
    if (is_accu_adc_result_ready()) {
        accu_adc_result = smooth_value(accu_adc_result, get_accu_adc_result());
        app_debug_status.accu_voltage_lo = accu_adc_result;
        app_debug_status.adc_voltage_hi = (app_debug_status.adc_voltage_hi & 0xF0) | (reverse_bytes(accu_adc_result) & 0x0F);
    }

    const uint16_t timer = get_timer_value();
    if (timer == next_adc_counter_read_timeline) {
        next_adc_counter_read_timeline = timer + INTERVAL_ONE_SECOND;
        app_debug_status.adc_count = reverse_bytes(exchange_adc_count());
    }
}

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

void launch_adc()
{
    // odpalamy co 6 ms
    switch (get_timer_value() & 3) {
        case 0:
        launch_bulb_adc();
        break;
        case 2:
        launch_accu_adc();
        break;
    }
}