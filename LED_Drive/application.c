/*
 * application.c
 *
 * Created: 05.03.2023 17:15:16
 *  Author: Bogusław
 */

#include <avr/io.h>
#include <stdbool.h>

#include "adc.h"
#include "pwm.h"
#include "pin_io.h"
#include "timer.h"

typedef enum beam_actual_status_e {
    // blisko zero V, przepalona żarówka lub bezpiczenik
    BEAM_VOLTAGE_ZERO,

    // około 200 mV (100 - 500), normalna praca
    BEAM_VOLTAGE_UNDER_LOAD,

    // powyżej 4 V, wszystko ok, żarówka wyłączona
    BEAM_VOLTAGE_FULL,

    BEAM_VOLTAGE_UNKNOWN_READING
} beam_actual_status;

inline void setup_adc()
{
    uint8_t timer = get_timer_value() & 0x3F;
    if(timer == 0) {
        // every 64 ticks = 192 ms
        launch_led_adc();
    } else
    if ((timer & 0x03) == 1) {
        // every 4 ticks = 12 ms
        launch_beam_adc();
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
#define ONE_SECOND_INTERVAL ((uint16_t)(1000 / 3))
// 5 seconds in 3ms ticks
#define FIVE_SECONDS_INTERVAL ((uint16_t)(5000 / 3))

// 1/5 second in 3ms ticks
#define FIFTH_SECOND_INTERVAL ((uint16_t)(200 / 3))
#define FOUR_FIFTH_SECOND_INTERVAL ((uint16_t)(ONE_SECOND_INTERVAL - FIFTH_SECOND_INTERVAL))
#define HALF_A_SECOND_INTERVAL ((uint16_t)(500 / 3))


// no less that 5% of beam power, no more than 95% of beam power (except 100%)
# define BEAM_PWM_MARGIN ((uint8_t)(256 * 5 / 100))
# define BEAM_PWM_MARGIN_END ((uint8_t)(256 - BEAM_PWM_MARGIN))

// 225 - 2x 13 = 229 values to go throught while brightening
// to stretch it over 5 seconds with 3ms intervals
// we need to advance every 7th cycle
#define BEAM_BRIGHTENING_INTERVAL (uint8_t)( FIVE_SECONDS_INTERVAL / ( 256 - 2 * BEAM_PWM_MARGIN ))

inline static void adjust_beam_pwm() {
    uint16_t timer =  get_timer_value();
    if (next_beam_status_change_at != timer) {
        return ;
    }
    switch (beam_status_changes) {
        case BEAM_WAITING:
            // waiting expired, start brightening
            start_beam_pwm(BEAM_PWM_MARGIN); // power 5%
            next_beam_status_change_at = BEAM_BRIGHTENING_INTERVAL + timer;
            beam_status_changes = BEAM_BRIGHTENING;
            break;
        case BEAM_BRIGHTENING:
            if (get_beam_pwm_duty_cycle() < BEAM_PWM_MARGIN_END) {
                next_beam_status_change_at = BEAM_BRIGHTENING_INTERVAL + timer;
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
    IGNITION_POWER
} engine_start_status;

static inline engine_start_status get_engine_start_status() {
    switch (get_ignition_starter_status()) {
        case 0: // pin values LL
            return NO_IGNITION;
        case 3: // pin values HH
            return IGNITION_POWER;
        default: // pin values LH or HL
            return STARTER_CRANKING;
    }
}

static engine_start_status last_engine_start_status;
static bool gear_was_ever_engaged;
static bool beam_state;
static bool beam_was_ever_on;
static bool force_beam_off;

inline static void execute_engine_start_changes() {
    engine_start_status current_engine_start_status = get_engine_start_status();
    bool target_beam_status;

    switch (current_engine_start_status) {
        case STARTER_CRANKING:
            target_beam_status = false;
            // ignore button actions
            exchange_button_release_flag();
            break;
        case NO_IGNITION:
            if (last_engine_start_status != NO_IGNITION) {
                beam_state = beam_was_ever_on = false;
            }
            if (exchange_button_release_flag()) {
                force_beam_off = false;
                beam_state = !beam_state;
            }
            target_beam_status = beam_state;
            gear_was_ever_engaged = is_gear_engaged();
            break;
        case IGNITION_POWER:
            if (is_gear_engaged()) {
                // gear engaged
                if (is_button_pressed() && !gear_was_ever_engaged) {
                    // check if we are pressing button while engaging gear
                    ignore_next_button_release();
                    force_beam_off = true;
                }
                beam_state = false;
                beam_was_ever_on = true;
                target_beam_status = !force_beam_off;
                gear_was_ever_engaged = true;
            } else {
                // neutral
                target_beam_status = (!force_beam_off) && (beam_was_ever_on || beam_state);
            }
            if (exchange_button_release_flag()) {
                beam_state = true;
                force_beam_off = target_beam_status;
            }
            break;
    }

    last_engine_start_status = current_engine_start_status;

    if (target_beam_status == false) {
        set_beam_on_off(false);
        beam_status_changes = BEAM_OFF;
    } else if (beam_status_changes == BEAM_OFF) {
        // jeżeli było WDR, to od razu idziemy do zapalonej
        // launch beam brightening after wait
        beam_status_changes = BEAM_WAITING;
        next_beam_status_change_at = HALF_A_SECOND_INTERVAL + get_timer_value();
    }
    // skasowanie WDR
}

static inline beam_actual_status map_beam(uint16_t beam_value)
{
    // TODO: ztablicować, PROGMEM, pętla po tablicy
    // 50 mv -> 10
    // 100 mV -> 20
    // 500 mv > 102
    // 1 V -> 205
    // 2,5 V -> 512
    // 4 V -> 820
    if (beam_value <= 10) {
        return BEAM_VOLTAGE_ZERO;
    }
    if (beam_value >= 20 && beam_value <= 205) {
        return BEAM_VOLTAGE_UNDER_LOAD;
    }
    if (beam_value >= 820) {
        return BEAM_VOLTAGE_FULL;
    }
    return BEAM_VOLTAGE_UNKNOWN_READING;
}
// TODO: to samo dla LEDów

typedef enum led_status_e {
    // initial on for 1 second
    LED_INITIAL,
    // fast blinking, 5 Hz (i.e. 200 ms on, 200 ms off)
    LED_FAST_BLINKING,
    // slow blinking, 1 Hz with 20% duty cycle (i.e. 200 ms on, 800 ms off)
    LED_SLOW_BLINKING,
    // slow blinking, 1 Hz (i.e. 500 ms on, 500 ms off)
    LED_MEDIUM_BLINKING,
    // on or off depending on beam status
    LED_FOLLOW_BEAM
} led_status;

static led_status current_led_status_value;
static uint16_t next_led_status_change_at;

static inline void execute_led_info_changes()
{
    uint16_t timer = get_timer_value();
    if (current_led_status_value == LED_INITIAL){
        set_led_on();
        if (timer > ONE_SECOND_INTERVAL) {
            current_led_status_value = LED_FOLLOW_BEAM;
            set_led_off();
        }
        return;
    }
    if (current_led_status_value != LED_FOLLOW_BEAM && next_led_status_change_at == timer) {
        if (is_led_on()) {
            set_led_off();
            if (current_led_status_value == LED_SLOW_BLINKING) {
                next_led_status_change_at = FOUR_FIFTH_SECOND_INTERVAL + timer;
            } else {
                next_led_status_change_at = FIFTH_SECOND_INTERVAL + timer;
            }
        } else {
            set_led_on();
            next_led_status_change_at = FIFTH_SECOND_INTERVAL + timer;
        }
        if (current_led_status_value == LED_MEDIUM_BLINKING) {
            next_led_status_change_at = HALF_A_SECOND_INTERVAL + timer;
        }
    }

    if ( (timer & 0x03) != 0x03 ) {
        // ADC is executed on (timer & 0x03) == 0x01, so we wait a little to get fresh results
        return ;
    }

    uint16_t beam_value = get_beam_adc_result();
    beam_actual_status beam_status_value = map_beam(beam_value);

    // beam on
    // - BEAM_VOLTAGE_UNDER_LOAD -> ok
    // - BEAM_VOLTAGE_ZERO -> bulb failure or fuse blown
    // beam off
    // - BEAM_VOLTAGE_FULL -> ok
    // - BEAM_VOLTAGE_ZERO -> bulb failure or fuse blown
    led_status target_led_status_value;
    switch (beam_status_changes) {
        case BEAM_ON:
            switch (beam_status_value) {
                case BEAM_VOLTAGE_UNDER_LOAD:
                    set_led_on();
                    target_led_status_value = LED_FOLLOW_BEAM;
                    break;
                case BEAM_VOLTAGE_ZERO:
                    target_led_status_value = LED_SLOW_BLINKING;
                    break;
                default:
                    target_led_status_value = LED_FAST_BLINKING;
                    break;
            }
            break;
        case BEAM_OFF:
            switch (beam_status_value) {
                case BEAM_VOLTAGE_FULL:
                    set_led_off();
                    target_led_status_value = LED_FOLLOW_BEAM;
                    break;
                case BEAM_VOLTAGE_ZERO:
                    target_led_status_value = LED_SLOW_BLINKING;
                    break;
                default:
                    target_led_status_value = LED_FAST_BLINKING;
                    break;
            }
            break;
        default:
            // beam change in progress
            target_led_status_value = LED_MEDIUM_BLINKING;
            break;
    }
    if (target_led_status_value != LED_FOLLOW_BEAM && current_led_status_value == LED_FOLLOW_BEAM) {
        if (target_led_status_value == LED_MEDIUM_BLINKING) {
            next_led_status_change_at = HALF_A_SECOND_INTERVAL + timer;
        } else {
            next_led_status_change_at = FIFTH_SECOND_INTERVAL + timer;
        }
        set_led_on();
    }
    current_led_status_value = target_led_status_value;
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

// do not store used registers on stack
void loop_application_logic(void)
    __attribute__((OS_main));

void loop_application_logic()
{
    read_pin_values();
    execute_state_transition_changes();
    adjust_pwm_values();
    setup_adc();
}
