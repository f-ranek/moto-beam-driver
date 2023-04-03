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

typedef enum beam_actual_status_e {
    // blisko zero V, przepalona żarówka lub bezpiczenik
    BEAM_VOLTAGE_ZERO,

    // około 200 mV (100 - 500), normalna praca
    BEAM_VOLTAGE_UNDER_LOAD,

    // powyżej 4 V, wszystko ok, żarówka wyłączona
    BEAM_VOLTAGE_FULL,

    BEAM_VOLTAGE_UNKNOWN_READING
} beam_actual_status;

static inline bool was_unexpected_reset() {
    uint8_t reset_flags = _BV(WDRF)
        // remove this flag once tests are done?
        | _BV(EXTRF);
    return ( MCUSR_initial_copy & reset_flags ) != 0;
}

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
#define ONE_TENTH_SECOND_INTERVAL ((uint16_t)(100 / 3))

// no less that 5% of beam power, no more than 95% of beam power (except 100%)
# define BEAM_PWM_MARGIN ((uint8_t)(256 * 5 / 100))
# define BEAM_PWM_MARGIN_END ((uint8_t)(256 - BEAM_PWM_MARGIN))

// 225 - 2x 13 = 229 values to go throught while brightening
// to stretch it over 5 seconds with 3ms intervals
// we need to advance every 7th cycle
#define BEAM_BRIGHTENING_INTERVAL (uint8_t)( FIVE_SECONDS_INTERVAL / ( 256 - 2 * BEAM_PWM_MARGIN ))

 static void adjust_beam_pwm() {
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
static bool force_beam_off;

static void execute_engine_start_changes() {
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
                beam_state = false;
            }
            if (exchange_button_release_flag()) {
                beam_state = !beam_state;
            }
            force_beam_off = false;
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
                target_beam_status = !force_beam_off;
                gear_was_ever_engaged = true;
            } else {
                // neutral
                target_beam_status = (!force_beam_off) && (gear_was_ever_engaged || beam_state);
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
        if (was_unexpected_reset()) {
            beam_status_changes = BEAM_ON;
            set_beam_on_off(1);
        } else {
            // launch beam brightening after wait
            beam_status_changes = BEAM_WAITING;
            next_beam_status_change_at = HALF_A_SECOND_INTERVAL + get_timer_value();
        }
    }
}

// 0 - 10   - ZERO
// 20 - 205 - UNDER_LOAD
// 820+     - FULL
static inline beam_actual_status map_beam(uint16_t beam_value)
{
    // 50 mv -> 10
    // 100 mV -> 20
    // 500 mv > 102
    // 1 V -> 205
    // 2,5 V -> 512
    // 4 V -> 820
    if (beam_value <= 10) {
        return BEAM_VOLTAGE_ZERO;
    }
    if (beam_value <= 19) {
        return BEAM_VOLTAGE_UNKNOWN_READING;
    }
    if (beam_value <= 205) {
        return BEAM_VOLTAGE_UNDER_LOAD;
    }
    if (beam_value <= 819) {
        return BEAM_VOLTAGE_UNKNOWN_READING;
    }
    return BEAM_VOLTAGE_FULL;
}

typedef enum led_status_e {
    // initial on for 1 second
    LED_INITIAL,
    // fast blinking, 5 Hz (i.e. 200 ms on, 200 ms off)
    // indicates unexpected device reset
    LED_INITIAL_FAST_BLINKING,
    // fast blinking, 5 Hz (i.e. 200 ms on, 200 ms off)
    LED_FAST_BLINKING,
    // slow blinking, 1 Hz with 20% duty cycle (i.e. 200 ms on, 800 ms off)
    LED_SLOW_BLINKING,
    // slow blinking, 0,5 Hz (i.e. 1 s on, 1 s off)
    LED_CLOCK_BLINKING,
    // on or off depending on beam status
    LED_FOLLOW_BEAM
} led_status;

static led_status current_led_status_value;
static uint16_t next_led_status_change_at;

// TODO: this is way too fucking hardcoded and complicated
static void execute_led_info_changes()
{
    uint16_t timer = get_timer_value();
    switch (current_led_status_value){
        case LED_INITIAL:
            if (was_unexpected_reset()) {
                current_led_status_value = LED_INITIAL_FAST_BLINKING;
                next_led_status_change_at = FIFTH_SECOND_INTERVAL + timer;
            }
            if (timer > ONE_SECOND_INTERVAL) {
                current_led_status_value = LED_FOLLOW_BEAM;
            } else {
                set_led_on();
            }
            return ;
        case LED_INITIAL_FAST_BLINKING:
           if (timer > 2*ONE_SECOND_INTERVAL-ONE_TENTH_SECOND_INTERVAL) { // without -1/10 sec here we have five blinks, and one very short blink
                current_led_status_value = LED_FOLLOW_BEAM;
            } else if (next_led_status_change_at == timer){
                next_led_status_change_at = FIFTH_SECOND_INTERVAL + timer;
                 if (is_led_on()) {
                     set_led_off();
                 } else {
                     set_led_on();
                 }
            }
            return ;
        default:
            break;
    }

    // TODO: this is too fucking hardcoded
    if (current_led_status_value != LED_FOLLOW_BEAM && next_led_status_change_at == timer) {
        // we have blinking in progress, and it's blink change time
        if (is_led_on()) {
            set_led_off();
            if (current_led_status_value == LED_SLOW_BLINKING) {
                // for slow blinking, off is 4/5 second
                next_led_status_change_at = FOUR_FIFTH_SECOND_INTERVAL + timer;
            } else {
                // for other kind of blinking (actually fast), off is 1/5 second
                next_led_status_change_at = FIFTH_SECOND_INTERVAL + timer;
            }
        } else {
            set_led_on();
            next_led_status_change_at = FIFTH_SECOND_INTERVAL + timer;
        }
        // override fo medium blinking, where timer is always 1/2 second
        if (current_led_status_value == LED_CLOCK_BLINKING) {
            next_led_status_change_at = HALF_A_SECOND_INTERVAL + timer;
        }

        return ;
    }

    if ( (timer & 0x03) != 0x03 ) {
        // ADC is executed on (timer & 0x03) == 0x01, so we wait a little to get fresh results
        return ;
    }

    // so we are in the state of LED_FOLLOW_BEAM

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
            target_led_status_value = LED_CLOCK_BLINKING;
            break;
    }
    if (target_led_status_value != LED_FOLLOW_BEAM && current_led_status_value == LED_FOLLOW_BEAM) {
        // we are about to blink. schedule led blink time
        if (target_led_status_value == LED_CLOCK_BLINKING) {
            next_led_status_change_at = ONE_SECOND_INTERVAL + timer;
        } else {
            next_led_status_change_at = FIFTH_SECOND_INTERVAL + timer;
        }
        set_led_on();
    }
    current_led_status_value = target_led_status_value;
}

#ifdef LED_ADC
static uint8_t led_buckets[5];
// static const uint8_t led_pwm_values[5] PROGMEM = { 10, 20, 50, 120, 255 };
// static const uint8_t led_pwm_values[5] PROGMEM = { 1, 2, 5, 12, 25 };

static const uint8_t led_pwm_values[5] PROGMEM = { 1, 4, 16, 64, 255 };


static void adjust_led_pwm()
{
    if (!exchange_led_adc_result_available()) {
        return ;
    }
    uint16_t led_result = get_led_adc_result();
    // 0 - 1.1 V reference
    // means 1 is more or less 1 mV
    // well, we are actually not on the light side of the force
    // very small values for dark and medium lightening conditions
    // very high values for bright ambient
    // nothing in between

    // add ADC conversion errors and we are doomed

    // 0..1 mV - dark
    // 3 mV - not dark
    // 100 mV - light
    // > 100 mV - sunny day
    uint8_t mapped_led_result;
    if (led_result <= 3) {
        mapped_led_result = 0;
    } else if (led_result <= 10) {
        mapped_led_result = 1;
    } else if (led_result <= 100) {
        mapped_led_result = 2;
    } else if (led_result <= 500) {
        mapped_led_result = 3;
    } else {
        mapped_led_result = 4;
    }

    // TODO: using 1 actually disables buckets
    if (++led_buckets[mapped_led_result] == 1) {
        // bucket overflows, it can happend every 10 seconds -- XXX
        memset(led_buckets, 0, sizeof(led_buckets));
        //led_buckets[0] = led_buckets[1] = led_buckets[2] = led_buckets[3] = led_buckets[4] = 0;
        uint8_t led_pwm_value = led_pwm_values[mapped_led_result];
        set_led_pwm(led_pwm_value);
    }
}
#endif // LED_ADC


static inline void execute_state_transition_changes()
{
    execute_engine_start_changes();
    execute_led_info_changes();
}

static inline void adjust_pwm_values()
{
    adjust_beam_pwm();
    #ifdef LED_ADC
    adjust_led_pwm();
    #endif // LED_ADC
}

// do not store used registers on stack
void loop_application_logic(void)
    __attribute__((OS_main));

inline void loop_application_logic()
{
    read_pin_values();
    execute_state_transition_changes();
    adjust_pwm_values();
    setup_adc();
    // was_unexpected_reset must return true (if should)
    // for some time after system start
    // until PINs reading stabilise

    // so it is allowed to read until first 16 x 3ms = 48 ms
    if ((get_timer_value() & 0x10) != 0) {
        MCUSR_initial_copy = 0;
    }
}
