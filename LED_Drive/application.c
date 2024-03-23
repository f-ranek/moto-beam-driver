/*
 * application.c
 *
 * Created: 05.03.2023 17:15:16
 *  Author: Bogusław
 */

#include <avr/io.h>
#include <string.h>
#include <stdbool.h>

#include "application.h"
#include "adc.h"
#include "pwm.h"
#include "pin_io.h"
#include "timer.h"
#include "main.h"
#include "spi.h"
#include "app_adc.h"
#include "morse.h"

static inline bool was_unexpected_reset() {
    const uint8_t reset_flags = _BV(WDRF)
        // remove this flag once tests are done?
        | _BV(EXTRF);
    return ( MCUSR_initial_copy & reset_flags ) != 0;
}

app_debug_status_t app_debug_status;

static enum app_state_e {
    // 0 - initial status, bulb off
    APP_INIT,

    // 1 - force off by user
    APP_FORCE_OFF,

    // 2 - transition between off and on
    APP_AUTO_BRIGHTENING,

    // 3 - bulb at proper power (13,2 V)
    APP_AUTO_ON,

    // 4 - still on, but waiting for being off
    APP_WAITING_FOR_BULB_OFF,

    // 5 - automatic off, subject to auto on
    APP_AUTO_OFF,

    // 6 - transition between off and on
    APP_FORCE_BRIGHTENING,

    // 7 - bulb at proper power (13,2 V)
    APP_FORCE_ON,

    // 8 - forced on, but starter running
    APP_FORCE_ON_STARTER
} app_state;

static inline bool is_oil_or_charging()
{
    accu_status_e accu_status;
    return is_oli_pressure() || ((accu_status = get_accu_status()) == CHARGING) || (accu_status == UNKNOWN);
}

#define BULB_BRIGHTENING_DELAY ((4000)/(3)/(225-2))

static uint16_t next_bulb_checkpoint;
static uint16_t next_led_checkpoint;

static void start_bulb_brightening()
{
    // TODO: sekunda przed rozpoczęciem rozjaśniania
    const uint16_t timer = get_timer_value();
    start_bulb_pwm(1);
    next_bulb_checkpoint = timer + BULB_BRIGHTENING_DELAY;

    set_led_on();
    next_led_checkpoint = timer + INTERVAL_HALF_A_SECOND;
}

static void turn_bulb_off_after_delay()
{
    app_state = APP_WAITING_FOR_BULB_OFF;
    const uint16_t timer = get_timer_value();
    next_bulb_checkpoint = timer + INTERVAL_FIVE_SECONDS;
}

// status początkowy
// 0
static void handle_app_init_state()
{
    // prawdopodobnie coś jebło, a jedziemy - od razu 100%
    if (was_unexpected_reset() && is_gear_engaged() && is_oil_or_charging()) {
        app_state = APP_AUTO_ON;
        start_bulb_pwm(220);
        set_led_on();
        return ;
    }

    const bool btn = exchange_button_release_flag();
    if (is_gear_engaged() && is_button_pressed()) {
        ignore_next_button_release();
        app_state = APP_FORCE_OFF;
        return ;
    }

    const bool oil_or_charging = is_oil_or_charging();
    if ((is_gear_engaged() && oil_or_charging) || (btn && oil_or_charging)) {
        start_bulb_brightening();
        app_state = APP_AUTO_BRIGHTENING;
        return ;
    }

    if (btn && !oil_or_charging) {
        start_bulb_brightening();
        app_state = APP_FORCE_BRIGHTENING;
        return ;
    }
}

// ręczne wyłączenie
// 1
static void handle_app_force_off_state()
{
    // tylko naciśnięcie włącza
    const bool btn = exchange_button_release_flag();
    const bool oil_or_charging = is_oil_or_charging();
    if (btn && oil_or_charging) {
        start_bulb_brightening();
        app_state = APP_AUTO_BRIGHTENING;
        return ;
    }
    if (btn && !oil_or_charging) {
        start_bulb_brightening();
        app_state = APP_FORCE_BRIGHTENING;
        return ;
    }
}

// rozjaśnianie
// 2
static void handle_app_auto_brightening_state()
{
    const uint16_t timer = get_timer_value();

    // naciśnięcie wyłącza
    if (exchange_button_release_flag() || exchange_was_btn_hold_for_1_sec()) {
        ignore_next_button_release();
        app_state = APP_FORCE_OFF;
        set_bulb_on_off(false);
        next_led_checkpoint = timer + INTERVAL_HALF_A_SECOND;
        set_led_off();
        return ;
    }

    if (get_accu_status() == STARTER_RUNNING) {
        app_state = APP_AUTO_OFF;
        set_bulb_on_off(false);
        return ;
    }

    if (timer == next_bulb_checkpoint) {
        // TODO: sekunda przed rozpoczęciem rozjaśniania
        // rozjaśnianie
        uint8_t current_power = get_bulb_pwm_duty_cycle();
        if (current_power < 220) {
            adjust_bulb_power(current_power + 1);
            next_bulb_checkpoint = timer + BULB_BRIGHTENING_DELAY;
        } else {
            // koniec rozjaśniania
            app_state = APP_AUTO_ON;
        }
    }
}

// żarówka włączona
// 3
static void handle_app_auto_on_state()
{
    const uint16_t timer = get_timer_value();

    // naciśnięcie wyłącza
    if (exchange_button_release_flag() || exchange_was_btn_hold_for_1_sec()) {
        ignore_next_button_release();
        app_state = APP_FORCE_OFF;
        set_bulb_on_off(false);
        next_led_checkpoint = timer + INTERVAL_HALF_A_SECOND;
        set_led_off();
        return ;
    }

    const accu_status_e accu_status = get_accu_status();
    if (accu_status == STARTER_RUNNING) {
        app_state = APP_AUTO_OFF;
        set_bulb_on_off(false);
        return ;
    }

    if (!is_oil_or_charging()) {
        turn_bulb_off_after_delay();
        return ;
    }

    // co 48 ms
    if ((timer & 0xF) == 0xF) {
        adjust_target_pwm_value_2(
            get_bulb_power(),
            &set_bulb_power);
    }
}

// żarówka za chwilę zgaśnie
// 4
static void handle_app_waiting_for_bulb_off_state()
{
    const uint16_t timer = get_timer_value();

    // naciśnięcie zostawia włączoną
    if (exchange_button_release_flag()) {
        app_state = APP_FORCE_ON;
        set_led_on();
        return ;
    }

    // przytrzymanie gasi od razu
    if (exchange_was_btn_hold_for_1_sec()) {
        ignore_next_button_release();
        app_state = APP_AUTO_OFF;
        set_bulb_on_off(false);
        next_led_checkpoint = timer + INTERVAL_HALF_A_SECOND;
        set_led_off();
        return ;
    }

    const accu_status_e accu_status = get_accu_status();
    if (accu_status == STARTER_RUNNING) {
        app_state = APP_AUTO_OFF;
        set_bulb_on_off(false);
        return ;
    }

    if (timer == next_bulb_checkpoint) {
        app_state = APP_AUTO_OFF;
        set_bulb_on_off(false);
        return ;
    }
}

// żarówka automatycznie wyłączona
// 5
static void handle_app_auto_off_state()
{
    if (exchange_button_release_flag()) {
        start_bulb_brightening();
        app_state = APP_FORCE_BRIGHTENING;
        return ;
    }
    if (is_gear_engaged() && is_oil_or_charging()) {
        start_bulb_brightening();
        app_state = APP_AUTO_BRIGHTENING;
        return ;
    }
}

// wymuszenie włączenia, rozjaśnia się
// 6
static void handle_app_force_brightening_state()
{
    const uint16_t timer = get_timer_value();

    // naciśnięcie wyłącza - bz.
    if (exchange_button_release_flag() || exchange_was_btn_hold_for_1_sec()) {
        ignore_next_button_release();
        app_state = APP_FORCE_OFF;
        set_bulb_on_off(false);
        next_led_checkpoint = timer + INTERVAL_HALF_A_SECOND;
        set_led_off();
        return ;
    }

    if (get_accu_status() == STARTER_RUNNING) {
        app_state = APP_FORCE_ON_STARTER; // zm.
        set_bulb_on_off(false);
        return ;
    }

    if (timer == next_bulb_checkpoint) {
        // TODO: sekunda przed rozpoczęciem rozjaśniania
        // rozjaśnianie
        uint8_t current_power = get_bulb_pwm_duty_cycle();
        if (current_power < 220) {
            adjust_bulb_power(current_power + 1);
            next_bulb_checkpoint = timer + BULB_BRIGHTENING_DELAY;
        } else {
            // koniec rozjaśniania
            app_state = APP_FORCE_ON; // zm.
         }
    }

}

// wymuszenie włączenia, włączona
// 7
static void handle_app_force_on_state()
{
    // naciśnięcie przełącza w tryb auto
    const bool btn = exchange_button_release_flag();

    if (exchange_was_btn_hold_for_1_sec()) {
        ignore_next_button_release();
        set_bulb_on_off(false);
        app_state = APP_FORCE_OFF;
        return ;
    }

    const bool oil_or_charging = is_oil_or_charging();
    const uint16_t timer = get_timer_value();

    if (btn && oil_or_charging) {
        set_led_off();
        next_led_checkpoint = timer + INTERVAL_FIFTH_SECOND;
        app_state = APP_AUTO_ON;
        return ;
    }

    if (btn && !oil_or_charging) {
        app_state = APP_AUTO_OFF;
        set_bulb_on_off(false);
        return ;
    }

    if (get_accu_status() == STARTER_RUNNING) {
        app_state = APP_FORCE_ON_STARTER;
        set_bulb_on_off(false);
        return ;
    }

    // co 48 ms
    if ((timer & 0xF) == 0xF) {
        adjust_target_pwm_value_2(
        get_bulb_power(),
        &set_bulb_power);
    }
}

// wymuszenie włączenia, ale kręci rozrusznik lub nie ma ładowania
// 8
static void handle_app_force_on_starter_state()
{
    // naciśnięcie przełącza w tryb auto
    const bool btn = exchange_button_release_flag();
    if (btn || is_oil_or_charging()) {
        start_bulb_brightening();
        app_state = APP_FORCE_BRIGHTENING;
        return ;
    }
    if (exchange_was_btn_hold_for_1_sec()) {
        app_state = APP_FORCE_OFF;
        set_bulb_on_off(false);
        set_led_on();
        next_led_checkpoint = get_timer_value() + INTERVAL_FIFTH_SECOND;
        return ;

    }
}

static inline bool is_bulb_off_voltage_ok()
{
    return get_bulb_actual_status() == BULB_VOLTAGE_FULL;
}

static inline bool is_bulb_on_voltage_ok()
{
    return get_bulb_actual_status() == BULB_VOLTAGE_UNDER_LOAD;
}

static void execute_state_transition_changes()
{
    switch (app_state) {
        case APP_INIT:
            // 0
            handle_app_init_state();
            break;
        case APP_FORCE_OFF:
            // 1
            handle_app_force_off_state();
            break;
        case APP_AUTO_BRIGHTENING:
            // 2
            handle_app_auto_brightening_state();
            break;
        case APP_AUTO_ON:
            // 3
            handle_app_auto_on_state();
            break;
        case APP_WAITING_FOR_BULB_OFF:
            // 4
            handle_app_waiting_for_bulb_off_state();
            break;
        case APP_AUTO_OFF:
            // 5
            handle_app_auto_off_state();
            break;
        case APP_FORCE_BRIGHTENING:
            // 6
            handle_app_force_brightening_state();
            break;
        case APP_FORCE_ON:
            // 7
            handle_app_force_on_state();
            break;
        case APP_FORCE_ON_STARTER:
            // 8
            handle_app_force_on_starter_state();
            break;
    }
}

static uint16_t blink_led(uint16_t on_duration, uint16_t off_duration)
{
    if (is_led_on()) {
        set_led_off();
        return off_duration;
    } else{
        set_led_on();
        return on_duration;
    }
}

static uint16_t execute_led_bulb_off()
{
    if (is_bulb_off_voltage_ok()) {
        set_led_off();
    } else {
        return blink_led(INTERVAL_FIFTH_SECOND, INTERVAL_FOUR_FIFTH_SECOND);
    }
    return 0;
}

static uint16_t execute_led_bulb_on()
{
    if (is_bulb_on_voltage_ok()) {
        set_led_on();
    } else {
        return blink_led(INTERVAL_19_20_SECOND, INTERVAL_1_20_SECOND);
    }
    return 0;
}

static uint16_t execute_led_bulb_transition_up()
{
    // miganie 50%
    return blink_led(INTERVAL_HALF_A_SECOND, INTERVAL_HALF_A_SECOND);
}

static uint16_t execute_led_bulb_transition_down()
{
    // miganie 80% / 20%
    return blink_led(INTERVAL_FOUR_FIFTH_SECOND, INTERVAL_FIFTH_SECOND);
}

static void execute_led_changes()
{
    const uint16_t timer = get_timer_value();
    if (get_accu_status() == STARTER_RUNNING) {
        set_led_off();
        next_led_checkpoint = timer + INTERVAL_FIFTH_SECOND;
        return ;
    }
    if (timer != next_led_checkpoint) {
        return ;
    }
    uint16_t led_interval = 0;
    switch (app_state) {
        case APP_INIT:
            // 0
        case APP_FORCE_OFF:
            // 1
        case APP_AUTO_OFF:
            // 5
        case APP_FORCE_ON_STARTER:
            // 8
            // - sprawdzenie napięcia - off
            led_interval = execute_led_bulb_off();
            break;
        case APP_AUTO_ON:
            // 3
        case APP_FORCE_ON:
            // 7
            // - sprawdzenie napięcia - on
            led_interval = execute_led_bulb_on();
            break;
        case APP_WAITING_FOR_BULB_OFF:
            // 4
            // miganie 4/5
            led_interval = execute_led_bulb_transition_down();
            break;
        case APP_AUTO_BRIGHTENING:
            // 2
        case APP_FORCE_BRIGHTENING:
            // 6
            // miganie 1/2
            led_interval = execute_led_bulb_transition_up();
            break;
    }
    next_led_checkpoint = timer + (led_interval == 0 ? INTERVAL_FIFTH_SECOND : led_interval);
}

static inline void emmit_debug_data()
{
    app_debug_status.bulb_pwm = get_bulb_power();
    app_debug_status.app_status = app_state;

    emmit_spi_data(&app_debug_status, sizeof(app_debug_status));
}

void loop_application_logic()
{
    read_pin_values();
    read_adc_results();
    execute_led_changes();
    execute_state_transition_changes();
    emmit_debug_data();
    emmit_morse_status_code_if_ready(app_state);
    launch_adc();

    // was_unexpected_reset must return true (if should)
    // for some time after system start
    // until PINs reading stabilise

    // so it is allowed to read until first 16 x 3ms = 48 ms
    if ((get_timer_value() & 0x10) != 0) {
        MCUSR_initial_copy = 0;
    }
}

void init_application() {
    next_led_checkpoint = INTERVAL_FIFTH_SECOND;
    // app_state = APP_INIT;
}
