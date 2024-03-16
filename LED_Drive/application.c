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

typedef enum bulb_status_change_e {
    // the bulb is off
    BULB_OFF,
    // in quiet period before starting lightening
    BULB_WAITING,
    // bean is getting brighter and brighter :)
    BULB_BRIGHTENING,
    // bulb is fully on
    BULB_ON
} bulb_status_change;

static uint16_t next_bulb_status_change_at;
static bulb_status_change bulb_status_changes;
*/

// 1 second in 3ms ticks
#define ONE_SECOND_INTERVAL ((uint16_t)(1000 / 3))
// 5 seconds in 3ms ticks
#define FIVE_SECONDS_INTERVAL ((uint16_t)(5000 / 3))

/*
// 1/5 second in 3ms ticks
#define FIFTH_SECOND_INTERVAL ((uint16_t)(200 / 3))
#define FOUR_FIFTH_SECOND_INTERVAL ((uint16_t)(ONE_SECOND_INTERVAL - FIFTH_SECOND_INTERVAL))
#define HALF_A_SECOND_INTERVAL ((uint16_t)(500 / 3))
#define ONE_TENTH_SECOND_INTERVAL ((uint16_t)(100 / 3))

// no less that 5% of bulb power, no more than 95% of bulb power (except 100%)
# define BULB_PWM_MARGIN ((uint8_t)(256 * 5 / 100))
# define BULB_PWM_MARGIN_END ((uint8_t)(256 - BULB_PWM_MARGIN))

// 225 - 2x 13 = 229 values to go throught while brightening
// to stretch it over 5 seconds with 3ms intervals
// we need to advance every 7th cycle
#define BULB_BRIGHTENING_INTERVAL (uint8_t)( FIVE_SECONDS_INTERVAL / ( 256 - 2 * BULB_PWM_MARGIN ))

static void adjust_bulb_pwm() {
    uint16_t timer =  get_timer_value();
    if (next_bulb_status_change_at != timer) {
        return ;
    }
    switch (bulb_status_changes) {
        case BULB_WAITING:
            // waiting expired, start brightening
            start_bulb_pwm(BULB_PWM_MARGIN); // power 5%
            next_bulb_status_change_at = BULB_BRIGHTENING_INTERVAL + timer;
            bulb_status_changes = BULB_BRIGHTENING;
            break;
        case BULB_BRIGHTENING:
            if (get_bulb_pwm_duty_cycle() < BULB_PWM_MARGIN_END) {
                next_bulb_status_change_at = BULB_BRIGHTENING_INTERVAL + timer;
                set_bulb_pwm(get_bulb_pwm_duty_cycle() + 1);
            } else {
                // we got 95% of power, go directly to 100%
                bulb_status_changes = BULB_ON;
                set_bulb_on_off(true);
            }
            break;
        case BULB_ON:
            break;
        case BULB_OFF:
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
static bool bulb_state;
static bool force_bulb_off;

static void execute_engine_start_changes() {
    engine_start_status current_engine_start_status = get_engine_start_status();
    bool target_bulb_status;

    switch (current_engine_start_status) {
        case STARTER_CRANKING:
            target_bulb_status = false;
            // ignore button actions
            exchange_button_release_flag();
            break;
        case NO_IGNITION:
            if (last_engine_start_status != NO_IGNITION) {
                bulb_state = false;
            }
            if (exchange_button_release_flag()) {
                bulb_state = !bulb_state;
            }
            force_bulb_off = false;
            target_bulb_status = bulb_state;
            gear_was_ever_engaged = is_gear_engaged();
            break;
        case IGNITION_POWER:
            if (is_gear_engaged()) {
                // gear engaged
                if (is_button_pressed() && !gear_was_ever_engaged) {
                    // check if we are pressing button while engaging gear
                    ignore_next_button_release();
                    force_bulb_off = true;
                }
                bulb_state = false;
                target_bulb_status = !force_bulb_off;
                gear_was_ever_engaged = true;
            } else {
                // neutral
                target_bulb_status = (!force_bulb_off) && (gear_was_ever_engaged || bulb_state);
            }
            if (exchange_button_release_flag()) {
                bulb_state = true;
                force_bulb_off = target_bulb_status;
            }
            break;
    }

    last_engine_start_status = current_engine_start_status;

    if (target_bulb_status == false) {
        set_bulb_on_off(false);
        bulb_status_changes = BULB_OFF;
    } else if (bulb_status_changes == BULB_OFF) {
        // jeżeli było WDR, to od razu idziemy do zapalonej
        if (was_unexpected_reset()) {
            bulb_status_changes = BULB_ON;
            set_bulb_on_off(true);
        } else {
            // launch bulb brightening after wait
            bulb_status_changes = BULB_WAITING;
            next_bulb_status_change_at = HALF_A_SECOND_INTERVAL + get_timer_value();
        }
    }
}

// 0 - 10   - ZERO
// 20 - 205 - UNDER_LOAD
// 820+     - FULL
static inline bulb_actual_status map_bulb(uint16_t bulb_value)
{
    // 50 mv -> 10
    // 100 mV -> 20
    // 500 mv > 102
    // 1 V -> 205
    // 2,5 V -> 512
    // 4 V -> 820
    if (bulb_value <= 10) {
        return BULB_VOLTAGE_ZERO;
    }
    if (bulb_value <= 19) {
        return BULB_VOLTAGE_UNKNOWN_READING;
    }
    if (bulb_value <= 205) {
        return BULB_VOLTAGE_UNDER_LOAD;
    }
    if (bulb_value <= 819) {
        return BULB_VOLTAGE_UNKNOWN_READING;
    }
    return BULB_VOLTAGE_FULL;
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
    // slow blinking, 1 Hz with 50% duty cycle (i.e. 0,5 s on, 0,5 s off)
    LED_CLOCK_BLINKING,
    // on or off depending on bulb status
    LED_FOLLOW_BULB
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
                current_led_status_value = LED_FOLLOW_BULB;
            } else {
                set_led_on();
            }
            return ;
        case LED_INITIAL_FAST_BLINKING:
           if (timer > 2*ONE_SECOND_INTERVAL-ONE_TENTH_SECOND_INTERVAL) { // without -1/10 sec here we have five blinks, and one very short blink
                current_led_status_value = LED_FOLLOW_BULB;
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
    if (current_led_status_value != LED_FOLLOW_BULB && next_led_status_change_at == timer) {
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
        // override fo medium blinking, where timer is always 1 second
        if (current_led_status_value == LED_CLOCK_BLINKING) {
            next_led_status_change_at = HALF_A_SECOND_INTERVAL + timer;
        }

        return ;
    }

    if ( (timer & 0x03) != 0x03 ) {
        // ADC is executed on (timer & 0x03) == 0x01, so we wait a little to get fresh results
        return ;
    }

    // so we are in the state of LED_FOLLOW_BULB

    uint16_t bulb_value = get_bulb_adc_result();
    bulb_actual_status bulb_status_value = map_bulb(bulb_value);

    // bulb on
    // - BULB_VOLTAGE_UNDER_LOAD -> ok
    // - BULB_VOLTAGE_ZERO -> bulb failure or fuse blown
    // bulb off
    // - BULB_VOLTAGE_FULL -> ok
    // - BULB_VOLTAGE_ZERO -> bulb failure or fuse blown
    led_status target_led_status_value;
    switch (bulb_status_changes) {
        case BULB_ON:
            switch (bulb_status_value) {
                case BULB_VOLTAGE_UNDER_LOAD:
                    set_led_on();
                    target_led_status_value = LED_FOLLOW_BULB;
                    break;
                case BULB_VOLTAGE_ZERO:
                    target_led_status_value = LED_SLOW_BLINKING;
                    break;
                default:
                    target_led_status_value = LED_FAST_BLINKING;
                    break;
            }
            break;
        case BULB_OFF:
            switch (bulb_status_value) {
                case BULB_VOLTAGE_FULL:
                    set_led_off();
                    target_led_status_value = LED_FOLLOW_BULB;
                    break;
                case BULB_VOLTAGE_ZERO:
                    target_led_status_value = LED_SLOW_BLINKING;
                    break;
                default:
                    target_led_status_value = LED_FAST_BLINKING;
                    break;
            }
            break;
        default:
            // bulb change in progress
            target_led_status_value = LED_CLOCK_BLINKING;
            break;
    }
    if (target_led_status_value != LED_FOLLOW_BULB && current_led_status_value == LED_FOLLOW_BULB) {
        // we are about to blink. schedule led blink time
        if (target_led_status_value == LED_CLOCK_BLINKING) {
            next_led_status_change_at = HALF_A_SECOND_INTERVAL + timer;
        } else {
            next_led_status_change_at = FIFTH_SECOND_INTERVAL + timer;
        }
        set_led_on();
    }
    current_led_status_value = target_led_status_value;
}


*/

typedef struct app_status_ {
    uint8_t adc_voltage_hi;     // bulb, accu - HI 4 bits
    uint8_t bulb_voltage_lo;
    uint8_t accu_voltage_lo;
    uint16_t adc_count;         // 4,5  - 6480 - 6503 ???
    uint8_t  bulb_pwm;          // 6
} app_status_t;


static uint8_t my_state;
static uint16_t next_adc_counter_read_timeline;

static app_status_t app_status;

static inline __attribute__ ((always_inline)) uint16_t reverse_bytes(uint16_t arg) {
    typedef uint8_t v2 __attribute__((vector_size (2)));
    v2 vec;
    vec[0] = arg >> 8;
    vec[1] = arg;
    return (uint16_t)vec;
}

#ifdef SIMULATION
extern void process_bulb_adc_result(uint16_t);
#endif // SIMULATION

uint16_t accu_adc_result;
uint16_t bulb_adc_result;

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

// tu by się przydało takie wyliczenie, że z bieżących wartości
// napięcia i pwm wyliczamy jakiś współczynnik korekcji
// i on później jest uwzględniany
static uint8_t calc_target_pwm_value(uint16_t accu_adc_result)
{
    if (accu_adc_result < VOLTAGE_13_V) {
        // napięcie AKU mniejsze niż 13 V
        return 255;
    }
    return 503 - (35 * accu_adc_result) / 128;
}

typedef void (*pwm_consumer_t)(uint8_t);

static inline void adjust_target_pwm_value(uint8_t current,
    uint16_t accu_adc_result, uint16_t bulb_adc_result,
    pwm_consumer_t pm_consumer)
{
    if (accu_adc_result < VOLTAGE_13_V || bulb_adc_result > VOLTAGE_6_V) {
        // napięcie AKU mniejsze niż 13 V
        // lub napięcie żarówki > 6 V
        pm_consumer(255);
        return ;
    }
    uint16_t actual_bulb_voltage = accu_adc_result - bulb_adc_result;
    /*
    if (actual_bulb_voltage >= VOLTAGE_13_1_V && actual_bulb_voltage <= VOLTAGE_13_2_V) {
        // nic nie trzeba robić
        return ;
    }*/

    if (actual_bulb_voltage < VOLTAGE_13_1_V) {
        // mniej niż 13,1 V - idziemy szybko do góry
        uint8_t target = calc_target_pwm_value(accu_adc_result);
        uint8_t diff;
        if (target > current + 3) {
            diff = (target - current) / 2;
        } else {
            diff = 1;
        }
        uint16_t target2 = (uint16_t)current + (uint16_t)diff;
        if (target2 > 255) {
            pm_consumer(255);
        } else {
            pm_consumer(target2 & 0xFF);
        }
        return ;
    }

    if (actual_bulb_voltage > VOLTAGE_13_2_V) {
        uint8_t target = calc_target_pwm_value(accu_adc_result);
        uint8_t diff;
        if (current > target + 3) {
            diff = (current - target) / 2;
        } else {
            diff = 1;
        }
        uint16_t target2 = (uint16_t)current - (uint16_t)diff;
        pm_consumer(target2 & 0xFF);
        return ;
    }

    // nic nie trzeba robić
}
/*
static void adjust_bulb_pwm(uint8_t pwm)
{
    if (pwm == 0) {
        set_bulb_on_off(false);
    } else if (pwm == 255) {
        set_bulb_on_off(true);
    } else {
        if (get_bulb_pwm_duty_cycle() == 0) {
            start_bulb_pwm(pwm);
        } else {
            adjust_bulb_pwm(pwm);
        }
    }
}
*/

static uint16_t next_bulb_brightening_checkpoint;
#define BULB_BRIGHTENING_DELAY ((5000)/(3)/(225-2))

static inline void execute_state_transition_changes()
{
    if (is_bulb_adc_result_ready()) {
        bulb_adc_result = get_bulb_adc_result();
        app_status.bulb_voltage_lo = bulb_adc_result;
        app_status.adc_voltage_hi = (app_status.adc_voltage_hi & 0x0F) | ((reverse_bytes(bulb_adc_result) & 0x0F) << 4);
    }
    if (is_accu_adc_result_ready()) {
        accu_adc_result = get_accu_adc_result();
        app_status.accu_voltage_lo = accu_adc_result;
        app_status.adc_voltage_hi = (app_status.adc_voltage_hi & 0xF0) | (reverse_bytes(accu_adc_result) & 0x0F);
    }

    const uint16_t timer = get_timer_value();
    if (timer == next_adc_counter_read_timeline) {
        next_adc_counter_read_timeline = timer + ONE_SECOND_INTERVAL;
        app_status.adc_count = reverse_bytes(exchange_adc_count());
    }

    if (my_state == 1 && timer == next_bulb_brightening_checkpoint) {
        // rozjaśnianie
        uint8_t current_power = get_bulb_pwm_duty_cycle();
        if ((current_power != 255) && (current_power < 225 || bulb_adc_result < VOLTAGE_13_1_V)) {
            // TODO: tutaj jest nie ok
            adjust_bulb_power(current_power + 1);
            next_bulb_brightening_checkpoint = timer + BULB_BRIGHTENING_DELAY;
        } else {
            // koniec rozjaśniania
            my_state = 2;
        }
    } else if (my_state == 2) {
        adjust_target_pwm_value(get_bulb_power(),
            accu_adc_result, bulb_adc_result,
            &set_bulb_power);
    }

    if (exchange_button_release_flag()) {
        // wyłączone
        if (my_state == 0) {
            my_state = 1;
            start_bulb_pwm(1);
            next_bulb_brightening_checkpoint = timer + BULB_BRIGHTENING_DELAY;
        } else /* if (my_state == 1 || my_state == 2) */ {
            set_bulb_on_off(false);
            my_state = 0;
        }
    }

    /*


    if (exchange_button_release_flag()) {

        my_state++;
        if (my_state == 51) {
            set_bulb_on_off(false);
            my_state = 0;
        } else if (my_state == 1) {
            start_bulb_pwm(5);
        } else if (my_state == 50) {
            set_bulb_on_off(true);
        } else {
            set_bulb_pwm(my_state*5);
        }
        ctr++;
    }*/

    app_status.bulb_pwm = get_bulb_power();

    emmit_spi_data(&app_status, sizeof(app_status));

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
/*

static inline void adjust_pwm_values()
{
    adjust_bulb_pwm();
    #ifdef LED_ADC
    adjust_led_pwm();
    #endif // LED_ADC
}*/

/*
#ifndef SIMULATION

// do not store used registers on stack
void loop_application_logic(void)
    __attribute__((OS_main));

#endif
*/

void loop_application_logic()
{
    read_pin_values();
    execute_state_transition_changes();
    //adjust_pwm_values();
    //setup_adc();
    // was_unexpected_reset must return true (if should)
    // for some time after system start
    // until PINs reading stabilise

    // so it is allowed to read until first 16 x 3ms = 48 ms
    if ((get_timer_value() & 0x10) != 0) {
        MCUSR_initial_copy = 0;
    }
}
