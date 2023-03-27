/*
 * application.c
 *
 * Created: 05.03.2023 17:15:16
 *  Author: Bogusław
 */

#include <avr/io.h>

#include "functions.h"
#include "pwm.h"
#include "pin_io.h"

typedef enum beam_actual_status_e {
    // blisko zero V, przepalona żarówka lub bezpiczenik
    BEAM_VAOLTAGE_ZERO,
    // około 200 mV (100 - 500), normalna praca
    BEAM_VAOLTAGE_UNDER_LOAD,

    // powyżej 4 V, wszystko ok, żarówka wyłączona
    BEAM_VAOLTAGE_FULL,

    BEAM_VAOLTAGE_UNKNOWN_READING
} beam_actual_status;

inline void setup_adc()
{
    uint8_t timer = get_timer_value() & 0x3F;
    // every 64 ticks = 192 ms
    if (timer == 0) {
        // every 384 ms
        launch_beam_adc();
    } else if(timer == 1) {
        // every 384 ms
        launch_led_adc();
    }
}

typedef enum beam_status_change_e {
    // the beam is off
    BEAM_OFF,
    // in quiet period before starting lightening
    BEAM_WAITING,
    // bean is getting brighter and brighter :)
    BEAM_BRIGHTENING,
    // beam is fully on
    BEAM_ON
} beam_status_change;

static uint16_t next_beam_status_change_at;
static beam_status_change beam_status_changes;

// 1 second in 3ms ticks
#define ONE_SECOND_INTERVAL ((uint16_t)(1000/3))
// 5 seconds in 3ms ticks
#define FIVE_SECONDS_INTERVAL ((uint16_t)(5000/3))

// no less that 5% of beam power, no more than 95% of beam power (except 100%)
# define BEAM_PWM_MARGIN ((uint8_t)(256*5/100))
# define BEAM_PWM_MARGIN_END ((uint8_t)(256-BEAM_PWM_MARGIN))

// 225 - 2x 13 = 229 values to go throught while brightening
// to stretch it over 5 seconds with 3ms intervals
// we need to advance every 7th cycle
#define BEAM_BRIGHTENING_INTERVAL (uint8_t)( FIVE_SECONDS_INTERVAL / ( 256 - 2* BEAM_PWM_MARGIN ))

inline static void adjust_beam_pwm() {
    if (next_beam_status_change_at != get_timer_value()) {
        return ;
    }
    switch (beam_status_changes) {
        case BEAM_WAITING:
            // waiting expired, start brightening
            start_beam_pwm(BEAM_PWM_MARGIN); // power 5%
            next_beam_status_change_at = BEAM_BRIGHTENING_INTERVAL + get_timer_value();
            beam_status_changes = BEAM_BRIGHTENING;
            break;
        case BEAM_BRIGHTENING:
            if (get_beam_pwm_duty_cycle() < BEAM_PWM_MARGIN_END) {
                next_beam_status_change_at = BEAM_BRIGHTENING_INTERVAL + get_timer_value();
                set_beam_pwm(get_beam_pwm_duty_cycle() + 1);
            } else {
                // we got 95% of power, go directly to 100%
                beam_status_changes = BEAM_ON;
                set_beam_on_off(1);
            }
            break;
        case BEAM_ON:
            break;
        case BEAM_OFF:
            break;
    }
}

typedef enum engine_start_status_e {
    // engine power is off = kill-switch in action
    NO_IGNITION,
    // starter in cranking
    STARTER_CRANKING,
    // engine is either running, or ready to be started
    IGNITION_ON
} engine_start_status;

static inline engine_start_status get_engine_start_status() {
    uint8_t ignition_starter_status = get_ignition_starter_status() & 0x03;
    if (ignition_starter_status == 0) { // pin values LL
        return NO_IGNITION;
    } else
    if (ignition_starter_status == 3) { // pin values HH
        return IGNITION_ON;
    } else { // pin values LH or HL
        return STARTER_CRANKING;
    }
}

static engine_start_status last_engine_start_status;
static uint8_t gear_was_ever_engaged;
static uint8_t beam_state;
static uint8_t beam_was_ever_on;
static uint8_t force_beam_off;

inline static void execute_engine_start_changes() {
    engine_start_status current_engine_start_status = get_engine_start_status();
    uint8_t target_beam_status;

    switch (current_engine_start_status) {
        case STARTER_CRANKING:
            target_beam_status = 0;
            // ignore button actions
            exchange_button_release_flag();
            break;
        case NO_IGNITION:
            if (last_engine_start_status != NO_IGNITION) {
                beam_state = beam_was_ever_on = 0;
            }
            if (exchange_button_release_flag()) {
                force_beam_off = 0;
                beam_state ^= 1;
            }
            target_beam_status = beam_state;
            gear_was_ever_engaged = is_gear_engaged();
            break;
        case IGNITION_ON:
            if (is_gear_engaged()) {
                // gear engaged
                if (is_button_pressed() && !gear_was_ever_engaged) {
                    // check if we are pressing button while engaging gear
                    ignore_next_button_release();
                    force_beam_off = 1;
                }
                beam_state = 0;
                beam_was_ever_on = 1;
                target_beam_status = !force_beam_off;
                gear_was_ever_engaged = 1;
            } else {
                // neutral
                target_beam_status = (!force_beam_off) & (beam_was_ever_on | beam_state);
            }
            if (exchange_button_release_flag()) {
                beam_state = 0;
                force_beam_off ^= 1;
            }
            break;
    }

    last_engine_start_status = current_engine_start_status;

    if (target_beam_status == 0) {
        set_beam_on_off(0);
        beam_status_changes = BEAM_OFF;
    } else if (beam_status_changes == BEAM_OFF) {
        beam_status_changes = BEAM_WAITING;
        next_beam_status_change_at = ONE_SECOND_INTERVAL + get_timer_value();
    }
}

static inline beam_actual_status map_beam(uint16_t beam_value)
{
    // TODO: ztablicować, PROGMEM, pętla po tablicy
    // 50 mv -> 10
    // 100 mV -> 20
    // 500 mv > 102
    // 4 V -> 820
    if (beam_value <= 10) {
        return BEAM_VAOLTAGE_ZERO;
    }
    if (beam_value >= 20 && beam_value <= 102) {
        return BEAM_VAOLTAGE_UNDER_LOAD;
    }
    if (beam_value >= 820) {
        return BEAM_VAOLTAGE_FULL;
    }
    return BEAM_VAOLTAGE_UNKNOWN_READING;
}

static inline void calculate_adc_readings()
{
    // TODO:
    // led_adc_result, beam_adc_result zawierają wartości od 0 do 1023
    // przy założeniu, że napięcie wejściowe to 5 V, każdy jeden bit odpowiada 4,88 mV
    // przy założeniu, że napięcie wejściowe to 4,75 V, każdy jeden bit odpowiada 4,64 mV
    //

    uint16_t beam_value = get_beam_adc_result();
    beam_actual_status beam_status_value = map_beam(beam_value);

    // TODO: to samo dla LEDów

    GPIOR0 = beam_status_value;
}

typedef enum led_status_e {
    LED_INITIAL,
    LED_WRONG_ADC_READING,
    LED_FOLLOW_BEAM
} led_status;

static led_status led_status_value;

static inline void execute_led_info_changes()
{
    if (led_status_value == LED_INITIAL) {
        set_led(1);
        if (get_timer_value() > ONE_SECOND_INTERVAL) {
            led_status_value = LED_FOLLOW_BEAM;
            set_led(0);
        }
    }
    // TODO: execute led
    // on startup -> on for one second, then off, else
    // if beam pwm in progress, then off, else
    // if wrong adc reading, then blink, else
    // follow beam status

    // beam on
    // - BEAM_VOLTAGE_UNDER_LOAD -> ok
    // beam off
    // - BEAM_VAOLTAGE_FULL -> ok
    // - BEAM_VAOLTAGE_ZERO -> bulb failure or fuse blown


}

static inline void execute_state_transition_changes()
{
    execute_engine_start_changes();
    execute_led_info_changes();
}

static inline void adjust_pwm_values()
{
    adjust_beam_pwm();
    // TODO: adjust LED pwm
}

void loop_application_logic()
{
    read_pin_values();
    calculate_adc_readings();
    execute_state_transition_changes();
    adjust_pwm_values();
    setup_adc();
}
