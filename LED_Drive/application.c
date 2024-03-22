/*
 * application.c
 *
 * Created: 05.03.2023 17:15:16
 *  Author: Bogusław
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <stdbool.h>

#include "adc.h"
#include "pwm.h"
#include "pin_io.h"
#include "timer.h"
#include "main.h"
#include "spi.h"
#include "app_adc.h"

/*
typedef enum bulb_actual_status_e {
    // blisko zero V, przepalona żarówka lub bezpiczenik
    BULB_VOLTAGE_ZERO,

    // około 200 mV (100 - 500), normalna praca
    BULB_VOLTAGE_UNDER_LOAD,

    // powyżej 4 V, wszystko ok, żarówka wyłączona
    BULB_VOLTAGE_FULL,

    BULB_VOLTAGE_UNKNOWN_READING
} bulb_actual_status;
*/

static inline bool was_unexpected_reset() {
    uint8_t reset_flags = _BV(WDRF)
        // remove this flag once tests are done?
        | _BV(EXTRF);
    return ( MCUSR_initial_copy & reset_flags ) != 0;
}

/*
static inline void setup_adc()
{
    uint8_t timer = get_timer_value();
    #ifdef LED_ADC
    if (timer == 0) {
        // every 256 ticks = 768 ms
        launch_led_adc();
    } else
    #endif // LED_ADC
    if ((timer & 0x03) == 1) {
        // every 4 ticks = 12 ms
        launch_bulb_adc();
    }
}

*/

// 1 second in 3ms ticks
#define ONE_SECOND_INTERVAL ((uint16_t)(1000 / 3))
// 5 seconds in 3ms ticks
#define FIVE_SECONDS_INTERVAL ((uint16_t)(5000 / 3))

// 1/5 second in 3ms ticks
#define FIFTH_SECOND_INTERVAL ((uint16_t)(200 / 3))
#define FOUR_FIFTH_SECOND_INTERVAL ((uint16_t)(ONE_SECOND_INTERVAL - FIFTH_SECOND_INTERVAL))
#define HALF_A_SECOND_INTERVAL ((uint16_t)(500 / 3))
#define ONE_TENTH_SECOND_INTERVAL ((uint16_t)(100 / 3))

static struct app_debug_status_t {
    uint8_t adc_voltage_hi;     // bulb, accu - HI 4 bits
    uint8_t bulb_voltage_lo;
    uint8_t accu_voltage_lo;
    uint8_t bulb_pwm;           // 4
    uint8_t app_status;         // 5
    uint16_t adc_count;         // 6,7  - 6480 - 6503 ???
} app_debug_status;

static inline __attribute__ ((always_inline)) uint16_t reverse_bytes(uint16_t arg) {
    typedef uint8_t v2 __attribute__((vector_size (2)));
    v2 vec;
    vec[0] = arg >> 8;
    vec[1] = arg;
    return (uint16_t)vec;
}


#define BULB_BRIGHTENING_DELAY ((4000)/(3)/(225-2))



static enum app_state_e {
    // initial status, bulb off
    INIT,
    // force off by user
    BULB_OFF,
    // transition between off and on
    BULB_BRIGHTENING,
    // bulb at proper power (13,2 V)
    BULB_ON,
    // still on, but waiting for being off
    WAITING_FOR_OFF,
    // automatic off, subject to auto on
    AUTO_OFF
} app_state;

static inline bool is_oil_or_charging()
{
    accu_status_e accu_status;
    return is_oli_pressure() || ((accu_status = get_accu_status()) == CHARGING) || (accu_status == UNKNOWN);
}

static uint16_t next_bulb_checkpoint;
static inline void start_bulb_brightening()
{
    // TODO: sekunda przed rozpoczęciem rozjaśniania
    const uint16_t timer = get_timer_value();
    start_bulb_pwm(1);
    next_bulb_checkpoint = timer + BULB_BRIGHTENING_DELAY;
    app_state = BULB_BRIGHTENING;
}

static inline void turn_bulb_off_after_delay()
{
    app_state = WAITING_FOR_OFF;
    const uint16_t timer = get_timer_value();
    next_bulb_checkpoint = timer + FIVE_SECONDS_INTERVAL;
}

// status początkowy
static inline void handle_init_status()
{
    if (exchange_button_release_flag() || is_oil_or_charging()) {
        start_bulb_brightening();
        return ;
    }
    if (is_gear_engaged() && exchange_button_pressed()) {
        app_state = BULB_OFF;
        return ;
    }
}

// ręczne wyłączenie
static inline void handle_bulb_off_status()
{
    if (exchange_button_release_flag()) {
        start_bulb_brightening();
        return ;
    }
}

// rozjaśnianie
static inline void handle_bulb_brightening_status()
{
    if (exchange_button_release_flag()) {
        app_state = BULB_OFF;
        set_bulb_on_off(false);
        return ;
    }

    if (get_accu_status() == STARTER_RUNNING) {
        app_state = AUTO_OFF;
        set_bulb_on_off(false);
        return ;
    }

    const uint16_t timer = get_timer_value();
    if (timer == next_bulb_checkpoint) {
        // TODO: sekunda przed rozpoczęciem rozjaśniania
        // rozjaśnianie
        uint8_t current_power = get_bulb_pwm_duty_cycle();
        if (current_power < 220) {
            adjust_bulb_power(current_power + 1);
            next_bulb_checkpoint = timer + BULB_BRIGHTENING_DELAY;
        } else {
            // koniec rozjaśniania
            app_state = BULB_ON;
        }
    }
}

// żarówka włączona
static inline void handle_bulb_on_status()
{
    if (exchange_button_release_flag()) {
        app_state = BULB_OFF;
        set_bulb_on_off(false);
        return ;
    }

    accu_status_e accu_status = get_accu_status();
    if (accu_status == STARTER_RUNNING) {
        app_state = AUTO_OFF;
        set_bulb_on_off(false);
        return ;
    }

    if (!is_oil_or_charging()) {
        turn_bulb_off_after_delay();
        return ;
    }

    // co 48 ms
    const uint16_t timer = get_timer_value();
    if ((timer & 0xF) == 0xF) {
        adjust_target_pwm_value_2(
            get_bulb_power(),
            &set_bulb_power);
    }
}

// żarówka za chwilę zgaśnie
static inline void handle_waiting_for_off_status()
{
    exchange_button_release_flag();
    // jak tu z przyciskiem?
    // przytrzymanie gasi
    // naciśnięcie zostawia włączoną
    const uint16_t timer = get_timer_value();
    if (timer == next_bulb_checkpoint) {
        app_state = BULB_OFF;
        set_bulb_on_off(false);
    }
}

// żarówka automatycznie wyłączona
static inline void handle_auto_off_status()
{
    bool btn = exchange_button_release_flag();
    if (btn || is_gear_engaged() || is_oil_or_charging()) {
        start_bulb_brightening();
        return ;
    }
}

static inline void execute_state_transition_changes()
{
    switch (app_state) {
        case INIT:
            handle_init_status();
            break;
        case BULB_OFF:
            handle_bulb_off_status();
            break;
        case BULB_BRIGHTENING:
            handle_bulb_brightening_status();
            break;
        case BULB_ON:
            handle_bulb_on_status();
            break;
        case WAITING_FOR_OFF:
            handle_waiting_for_off_status();
            break;
        case AUTO_OFF:
            handle_auto_off_status();
            break;
    }

    // TODO: stan diody
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
static inline void read_adc_results()
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
        next_adc_counter_read_timeline = timer + ONE_SECOND_INTERVAL;
        app_debug_status.adc_count = reverse_bytes(exchange_adc_count());
    }
}

static inline void emmit_debug_data()
{
    app_debug_status.bulb_pwm = get_bulb_power();
    app_debug_status.app_status = app_state;

    emmit_spi_data(&app_debug_status, sizeof(app_debug_status));
}

static inline void launch_adc()
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

void loop_application_logic()
{
    read_pin_values();
    read_adc_results();
    execute_state_transition_changes();
    emmit_debug_data();
    //adjust_pwm_values();
    launch_adc();
    // was_unexpected_reset must return true (if should)
    // for some time after system start
    // until PINs reading stabilise

    // so it is allowed to read until first 16 x 3ms = 48 ms
    if ((get_timer_value() & 0x10) != 0) {
        MCUSR_initial_copy = 0;
    }
}
