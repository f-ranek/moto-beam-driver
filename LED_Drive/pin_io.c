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
// PB0 - olej, brak ciśnienia = 0, jest = 1
// PB1 - luz = 0, bieg = 1

__pin_status __button_status;
__pin_status __motorcycle_status;
uint16_t next_btn_hold_checkpoint;

static inline void update_pin_status(__pin_status* status, uint8_t current_reading, uint8_t cycles);
static inline void read_button_value();

void read_pin_values()
{
    read_button_value();

    uint8_t motorcycle_status_reading = PINB & (_BV(0)|_BV(1));
    update_pin_status(&__motorcycle_status, motorcycle_status_reading, 9); // ~27 ms
}

static inline void read_button_value()
{
    // wduszony - PIN = 0
    bool last_button_status = __button_status.curr_status;
    bool button_reading = bit_is_set(PINA, 0) ? false : true;
    update_pin_status(&__button_status, button_reading, button_reading ? 15 : 5); // ~45 przy wciskaniu / 15 ms przy zwalnianiu
    bool new_button_status = __button_status.curr_status;
    // czy poprzednio był wduszony, a teraz jest zwolniony?
    if (last_button_status && !new_button_status) {
        __button_interrupt_pending |= _BV(0);
    }
    if (last_button_status == new_button_status) {
        if ((__button_interrupt_pending & _BV(3)) == 0) {
            __button_interrupt_pending |= _BV(3);
            next_btn_hold_checkpoint = get_timer_value() + INTERVAL_ONE_SECOND;
        } else if (next_btn_hold_checkpoint == get_timer_value()) {
            __button_interrupt_pending |= _BV(2);
        }
    } else {
        __button_interrupt_pending &= ~_BV(2);
        __button_interrupt_pending &= ~_BV(3);
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