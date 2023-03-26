/*
 * application.c
 *
 * Created: 05.03.2023 17:15:16
 *  Author: Bogusław
 */

#include <avr/io.h>

#include "functions.h"

typedef enum low_beam_status_e {
    // blisko zero V, przepalona żarówka lub bezpiczenik
    ZERO,
    // około 200 mV (100 - 500), normalna praca
    UNDER_LOAD,

    // powyżej 4 V, wszystko ok, żarówka wyłączona
    FULL,

    UNKNOWN_READING
} low_beam_status;

inline void setup_adc()
{
    uint8_t timer = get_timer_value() & 0x3F;
    // every 64 ticks = 192 ms
    if (timer == 0) {
        // every 384 ms
        launch_low_beam_adc();
    } else if(timer == 1) {
        // every 384 ms
        launch_led_adc();
    }
}

void loop_application_logic()
{
    read_pin_values();
    calculate_adc_readings();
    execute_state_transition_changes();
    adjust_pwm_values();
    setup_adc();
}

void execute_state_transition_changes()
{
    // todo: inicjalny status aplikacji?
    // todo: do wykonania dopiero, gdy licznik przkroczy np. 5? - żeby mieć odczytane wartości pinów??
    if (GPIOR1 == 1) {
        set_led(0);
        set_led(1);
        set_low_beam_on_off(0);
    }
    GPIOR2 = is_first_pass();
}

void adjust_pwm_values()
{
    // TODO:
    // 80%
    set_led_pwm(204);
    // 50%
    set_low_beam_pwm(128);
}

inline low_beam_status map_low_beam(uint16_t low_beam_value)
{
    // 50 mv -> 10
    // 100 mV -> 20
    // 500 mv > 102
    // 4 V -> 820
    if (low_beam_value <= 10) {
        return ZERO;
    }
    if (low_beam_value >= 20 && low_beam_value <= 102) {
        return UNDER_LOAD;
    }
    if (low_beam_value >= 820) {
        return FULL;
    }
    return UNKNOWN_READING;
}

void calculate_adc_readings()
{
    // TODO:
    // led_adc_result, low_beam_adc_result zawierają wartości od 0 do 1023
    // przy założeniu, że napięcie wejściowe to 5 V, każdy jeden bit odpowiada 4,88 mV
    // przy założeniu, że napięcie wejściowe to 4,75 V, każdy jeden bit odpowiada 4,64 mV
    //

    uint16_t low_beam_value = get_low_beam_adc_result();
    low_beam_status low_beam_status_value = map_low_beam(low_beam_value);

    GPIOR0 = low_beam_status_value;
}
