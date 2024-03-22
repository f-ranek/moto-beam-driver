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

#include "application.h"
#include "adc.h"
#include "pwm.h"
#include "pin_io.h"
#include "timer.h"
#include "main.h"
#include "spi.h"
#include "app_adc.h"

static inline bool was_unexpected_reset() {
    uint8_t reset_flags = _BV(WDRF)
        // remove this flag once tests are done?
        | _BV(EXTRF);
    return ( MCUSR_initial_copy & reset_flags ) != 0;
}

app_debug_status_t app_debug_status;



static enum app_state_e {
    // initial status, bulb off
    APP_INIT,
    // force off by user
    APP_BULB_OFF,
    // transition between off and on
    APP_BULB_BRIGHTENING,
    // bulb at proper power (13,2 V)
    APP_BULB_ON,
    // still on, but waiting for being off
    APP_WAITING_FOR_BULB_OFF,
    // automatic off, subject to auto on
    APP_AUTO_OFF
} app_state;

enum led_state_e {
    LED_OFF,
    LED_ON,
    LED_BULB_BRIGHTENING,
    LED_BULB_ERROR
} led_state;

static inline bool is_oil_or_charging()
{
    accu_status_e accu_status;
    return is_oli_pressure() || ((accu_status = get_accu_status()) == CHARGING) || (accu_status == UNKNOWN);
}

#define BULB_BRIGHTENING_DELAY ((4000)/(3)/(225-2))

static uint16_t next_bulb_checkpoint;
static uint16_t next_led_checkpoint;
static inline void start_bulb_brightening()
{
    // TODO: sekunda przed rozpoczęciem rozjaśniania
    const uint16_t timer = get_timer_value();
    start_bulb_pwm(1);
    next_bulb_checkpoint = timer + BULB_BRIGHTENING_DELAY;
    app_state = APP_BULB_BRIGHTENING;
}

static inline void turn_bulb_off_after_delay()
{
    app_state = APP_WAITING_FOR_BULB_OFF;
    const uint16_t timer = get_timer_value();
    next_bulb_checkpoint = timer + INTERVAL_FIVE_SECONDS;
}

// status początkowy
static inline void handle_app_init_state()
{
    // prawdopodobnie coś jebło, a jedziemy - od razu 100%
    if (was_unexpected_reset() && is_gear_engaged() && is_oil_or_charging()) {
        app_state = APP_BULB_ON;
        start_bulb_pwm(220);
        return ;
    }
    bool btn = exchange_button_release_flag();
    if (btn || is_oil_or_charging()) {
        start_bulb_brightening();
        return ;
    }
    if (is_gear_engaged() && exchange_button_pressed()) {
        app_state = APP_BULB_OFF;
        return ;
    }
}

// ręczne wyłączenie
static inline void handle_app_bulb_off_state()
{
    // tylko naciśnięcie włącza
    if (exchange_button_release_flag()) {
        start_bulb_brightening();
        return ;
    }
}

// rozjaśnianie
static inline void handle_app_bulb_brightening_state()
{
    // naciśnięcie wyłącza
    if (exchange_button_release_flag()) {
        app_state = APP_BULB_OFF;
        set_bulb_on_off(false);
        return ;
    }

    if (get_accu_status() == STARTER_RUNNING) {
        app_state = APP_AUTO_OFF;
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
            app_state = APP_BULB_ON;
        }
    }
}

// żarówka włączona
static inline void handle_app_bulb_on_state()
{
    // naciśnięcie wyłącza
    if (exchange_button_release_flag()) {
        app_state = APP_BULB_OFF;
        set_bulb_on_off(false);
        return ;
    }

    accu_status_e accu_status = get_accu_status();
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
    const uint16_t timer = get_timer_value();
    if ((timer & 0xF) == 0xF) {
        adjust_target_pwm_value_2(
            get_bulb_power(),
            &set_bulb_power);
    }
}



// żarówka za chwilę zgaśnie
static inline void handle_app_waiting_for_bulb_off_state()
{
    // naciśnięcie zostawia włączoną
    if (exchange_button_release_flag()) {
        app_state = APP_BULB_ON;
        return ;
    }
    // przytrzymanie gasi od razu
    if (exchange_was_hold_for_1_sec()) {
        app_state = APP_AUTO_OFF;
        set_bulb_on_off(false);
        return ;
    }

    const uint16_t timer = get_timer_value();
    if (timer == next_bulb_checkpoint) {
        app_state = APP_AUTO_OFF;
        set_bulb_on_off(false);
        return ;
    }
}

// żarówka automatycznie wyłączona
static inline void handle_app_auto_off_state()
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
        case APP_INIT:
            handle_app_init_state();
            break;
        case APP_BULB_OFF:
            handle_app_bulb_off_state();
            break;
        case APP_BULB_BRIGHTENING:
            handle_app_bulb_brightening_state();
            break;
        case APP_BULB_ON:
            handle_app_bulb_on_state();
            break;
        case APP_WAITING_FOR_BULB_OFF:
            handle_app_waiting_for_bulb_off_state();
            break;
        case APP_AUTO_OFF:
            handle_app_auto_off_state();
            break;
    }

    // TODO: stan diody

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
    execute_state_transition_changes();
    emmit_debug_data();
    launch_adc();

    // was_unexpected_reset must return true (if should)
    // for some time after system start
    // until PINs reading stabilise

    // so it is allowed to read until first 16 x 3ms = 48 ms
    if ((get_timer_value() & 0x10) != 0) {
        MCUSR_initial_copy = 0;
    }
}
