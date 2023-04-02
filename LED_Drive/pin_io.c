/*
 * pin_io.c
 *
 * Created: 05.03.2023 17:13:40
 *  Author: Bogusław
 */


#include <avr/io.h>
#include "timer.h"
#include "pin_io.h"

// PA0 - przycisk, zwolniony = 1 , wduszony = 0
// PA1 - luz = 0, bieg = 1
// PB0 - zapłon
// PB1 - rozrusznik

__pin_status __button_status;
__pin_status __ignition_starter_status;
__pin_status __neutral_status;

static inline void update_pin_status(__pin_status* status, uint8_t current_reading, uint8_t cycles);
static inline void read_button_value();

void read_pin_values()
{
    read_button_value();

    uint8_t neutral_reading = bit_is_set(PINA, 1) ? 1 : 0;
    update_pin_status(&__neutral_status, neutral_reading, 3);

    uint8_t ignition_starter_reading = PINB & (_BV(0)|_BV(1));
    update_pin_status(&__ignition_starter_status, ignition_starter_reading, 3); // ~9 ms
}

static inline void read_button_value()
{
    // wduszony - PIN = 0
    bool button_reading = bit_is_set(PINA, 0) ? 0 : 1;
    update_pin_status(&__button_status, button_reading, 15); // ~45 ms
    bool last_button_status = __button_status.curr_status;
    // czy poprzednio był wduszony, a teraz jest zwolniony?
    if (last_button_status && button_reading == 0) {
        __button_interrupt_pending |= _BV(0);
    }
}

// aktualizuje bieżący stan pinu, jeżeli stan wejścia nie zmienił się od cycles cykli wywołania
// jeżeli stan wejścia zmienił się, resetuje licznik
static inline void update_pin_status(__pin_status* status, uint8_t current_reading, uint8_t cycles)
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
        if (time_diff >= cycles) {
            // ok, pin już nie baunsuje
            status->curr_status = current_reading;
            status->temp_status = current_reading;
        }

    } else {
        status->temp_status = current_reading;
    }
} 