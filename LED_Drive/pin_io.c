/*
 * pin_io.c
 *
 * Created: 05.03.2023 17:13:40
 *  Author: Bogusław
 */


#include <avr/io.h>

#include "functions.h"

// PA0 - przycisk, zwolniony = 1 , wduszony = 0
// PA1 - luz = 0, bieg = 1
// PB0 - zapłon
// PB1 - rozrusznik

static __pin_status __button_status;
__pin_status __ignition_starter_status;
__pin_status __neutral_status;
uint8_t __button_interrupt_pending;

static inline void update_pin_status(__pin_status* status, uint8_t current_reading);
static inline void read_button_value();

void read_pin_values()
{
    read_button_value();

    uint8_t neutral_reading = (bit_is_set(PINA, 1) ? 1 : 0);
    update_pin_status(&__neutral_status, neutral_reading);

    uint8_t ignition_starter_reading = PINB & (_BV(0)|_BV(1));
    update_pin_status(&__ignition_starter_status, ignition_starter_reading);
}

static inline void read_button_value()
{
    uint8_t button_reading = (bit_is_set(PINA, 0) ? 1 : 0);
    uint8_t previous_button_status = __button_status.curr_status;
    update_pin_status(&__button_status, button_reading);
    uint8_t current_button_status = __button_status.curr_status;
    // czy poprzednio był wduszony, a teraz jest zwolniony?
    if (previous_button_status == 0 && current_button_status == 1) {
        __button_interrupt_pending = 1;
    }
}

// aktualizuje bieżący stan pinu, jeżeli stan wejścia nie zmienił się od 3 cykli wywołania
static inline void update_pin_status(__pin_status* status, uint8_t current_reading)
{
    if (current_reading != status->curr_status) {
        // odczytana wartość pinu jest inna, niż obecnie aktywna dla aplikacji
        uint8_t timer = get_timer_value();
        if (status->temp_status != current_reading) {
            // pin baunsuje, zresetuj zegar stabilnego stanu
            status->temp_status_since = timer;
            status->temp_status = current_reading;
            return ;
        }
        // pin jest stabilny, sprawdź jak długo
        uint8_t time_diff = timer - status->temp_status_since;
        if (time_diff >= 3) {
            // ok, pin już nie baunsuje
            status->curr_status = current_reading;
            status->temp_status = current_reading;
        }

    } else {
        status->temp_status = current_reading;
    }
} 