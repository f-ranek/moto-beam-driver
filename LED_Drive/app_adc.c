/*
 * app_adc.c
 *
 * Created: 22.03.2024 13:29:31
 *  Author: Bogusław
 */

#include <avr/pgmspace.h>

#include "app_adc.h"
#include "adc.h"
#include "application.h"
#include "timer.h"

static uint16_t accu_adc_result;
accu_status_e __accu_status_value;

static uint16_t bulb_adc_result;
static uint8_t twilight_adc_result;

static inline accu_status_e calc_accu_status(uint16_t accu_adc_result)
{
    if (accu_adc_result < VOLTAGE_6_V) {
        return UNKNOWN;
    }
    if (accu_adc_result < VOLTAGE_10_5_V) {
        return STARTER_RUNNING;
    }
    if (accu_adc_result < VOLTAGE_13_V) {
        return NORMAL;
    }
    return CHARGING;
}

bulb_actual_status_e get_bulb_actual_status()
{
    if (bulb_adc_result <= VOLTAGE_0_03_V) {
        // teoretycznie, przy IFRZ44N
        // oporność w stanie przewodzenia 17 mOhm
        // przy żarówce 55 W pobierającej 4 A
        // to daje niecałe 70 mV
        // a jeszcze weźmy pod uwagę błędy ADC
        //
        // dla IRF540N było 44 mOhm,
        // ale z jakiegoś powodu wychodził
        // większy spadek napięcia
        return BULB_VOLTAGE_ZERO;
    }
    if (bulb_adc_result < VOLTAGE_3_V) {
        return BULB_VOLTAGE_UNDER_LOAD;
    }
    if (bulb_adc_result > VOLTAGE_10_V) {
        return BULB_VOLTAGE_FULL;
    }
    return BULB_VOLTAGE_UNKNOWN_READING;
}


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
        return (new_val + old_val) / 2;
    }
}

static uint16_t next_adc_counter_read_timeline;
void read_adc_results()
{
    if (exchange_bulb_adc_result_ready()) {
        bulb_adc_result = smooth_value(bulb_adc_result, get_bulb_adc_result());
        app_debug_status.bulb_voltage_lo = bulb_adc_result;
        app_debug_status.adc_voltage_hi = (app_debug_status.adc_voltage_hi & 0x0F) | ((reverse_bytes(bulb_adc_result) & 0x0F) << 4);
    }
    if (exchange_accu_adc_result_ready()) {
        accu_adc_result = smooth_value(accu_adc_result, get_accu_adc_result());
        __accu_status_value = calc_accu_status(accu_adc_result);
        app_debug_status.accu_voltage_lo = accu_adc_result;
        app_debug_status.adc_voltage_hi = (app_debug_status.adc_voltage_hi & 0xF0) | (reverse_bytes(accu_adc_result) & 0x0F);
    }

    if (exchange_twilight_adc_result_ready()) {
        twilight_adc_result = get_twilight_adc_result();
        app_debug_status.twilight_voltage = twilight_adc_result;
    }

    const uint16_t timer = get_timer_value();
    if (timer == next_adc_counter_read_timeline) {
        next_adc_counter_read_timeline = timer + INTERVAL_ONE_SECOND;
        app_debug_status.adc_count = reverse_bytes(exchange_adc_count());
    }
}

// static uint8_t calc_target_pwm_value(uint16_t accu_adc_result)
// {
//     if (accu_adc_result < VOLTAGE_13_V) {
//         // napięcie AKU mniejsze niż około 13 V
//         return 255;
//     }
//     if (accu_adc_result >= VOLTAGE_15_V) {
//         // napięcie AKU większe niż 15 V
//         return 198;
//     }
//     // y = -0,4725𝑥+681,01
//     __uint24 buffer = accu_adc_result;
//     buffer *= 4725;
//     buffer = 6810100 - buffer;
//     buffer /= 10000;
//     int16_t result = buffer;
//     /*
//     float buffer = accu_adc_result;
//     buffer *= -0.4725;
//     int16_t result = buffer;
//     result += 681;
//     */
//     if (result > 255) {
//         return 255;
//     }
//     if (result < 198) {
//         return 198;
//     }
//     return result;
// }

static const uint8_t PWM_VALUES[] PROGMEM = {
    254, 254, 253, 253, 252, 252, 251, 251, 250, 250, 249, 249, 248, 248, 247, 246,
    246, 245, 244, 243, 243, 242, 242, 242, 242, 241, 241, 241, 240, 240, 239, 239,
    238, 238, 237, 237, 236, 235, 234, 234, 233, 233, 232, 232, 231, 231, 230, 230,
    230, 229, 229, 228, 228, 227, 227, 227, 226, 226, 225, 225, 224, 223, 223, 222,
    222, 221, 221, 220, 220, 219, 219, 219, 219, 218, 218, 218, 217, 217, 216, 216,
    215, 215, 214, 214, 213, 213, 212, 212, 211, 211, 210, 210, 210, 210, 209, 209,
    209, 208, 208, 207, 207, 206, 206, 206, 205, 205, 204, 204, 203, 203, 202, 202,
    201, 201, 200, 200, 200, 199, 199, 199
};

static uint8_t pwm_cache_key_0 = 0xFF;
static uint8_t pwm_cache_value_0;
static uint8_t pwm_cache_key_1 = 0xFF;
static uint8_t pwm_cache_value_1;

void adjust_target_pwm_value_impl(
    uint16_t adc,
    pwm_consumer_t pm_consumer)
{
    if (adc < VOLTAGE_6_V) {
        return;
    }
    if (adc > VOLTAGE_15_V) {
        pm_consumer(198);
        return;
    }

    if (adc <= 903) {
        pm_consumer(255);
        return;
    }

    const uint8_t index = adc - 904;

    if (pwm_cache_key_0 == index) {
        pm_consumer(pwm_cache_value_0);
        return;
    }
    if (pwm_cache_key_1 == index) {
        pm_consumer(pwm_cache_value_1);
        return;
    }
    if (index < sizeof(PWM_VALUES)) {
        uint8_t result = pgm_read_byte(&(PWM_VALUES[index]));
        pwm_cache_key_1 = pwm_cache_key_0;
        pwm_cache_value_1 = pwm_cache_value_0;
        pwm_cache_key_0 = index;
        pwm_cache_value_0 = result;
        pm_consumer(result);
    }
}

void adjust_target_pwm_value(
    pwm_consumer_t pm_consumer)
{
    adjust_target_pwm_value_impl(accu_adc_result, pm_consumer);
}

static uint8_t adc_launch_selector;

void launch_adc()
{
    // accu - 7 ms   - 3 cykle
    // bulb - 8,5 ms - 3 cykle
    // twlg - 1,2 ms - 1 cykl

    if (!is_adc_ready()) {
        return ;
    }

    // twlg co 576 ms...
    if (adc_launch_selector == 32) {
        launch_twilight_adc();
        adc_launch_selector = 0;
    } else {
        // pozostałe co 9 ms - na zmianę
        if ((++adc_launch_selector) % 2 == 1) {
            launch_accu_adc();
        } else {
            launch_bulb_adc();
        }
    }
}
