/*
 * pin_io.h
 *
 * Created: 27.03.2023 22:40:54
 *  Author: Bogusław
 */


#ifndef PIN_IO_H_
#define PIN_IO_H_

#include <avr/io.h>
#include <stdbool.h>

// read stable pin values
extern inline void read_pin_values();

typedef struct __pin_status_s {
    uint8_t temp_status_since;
    uint8_t curr_status;
    uint8_t temp_status;
} __pin_status;

extern __pin_status __button_status;
extern __pin_status __motorcycle_status;

// bit 0 - czy mamy przerwanie od przycisku
// bit 1 - czy następne przerwanie należy zignorować
#define __button_interrupt_pending GPIOR0

// zwraca bit opisujący stan ciśnienia oleju
// 0 -> brak ciśnienia
// 1 -> jest ciśnienie
static inline bool is_oli_pressure() {
    return (bool)((__motorcycle_status.curr_status & _BV(0)) != 0);
}

// zwraca bit opisujący stan wejścia biegu neutralnego
// 0 -> bieg neutralny
// 1 -> dowolny inny bieg
static inline bool is_gear_engaged() {
    return (bool)((__motorcycle_status.curr_status & _BV(1)) != 0);
}

// zwraca oraz resetuje flagę informującą o zwolnieniu przycisku
static inline bool exchange_button_release_flag() {
    uint8_t result = __button_interrupt_pending;
    if (result == 3) {
        // both interrupt and flag are set, reset both
        __button_interrupt_pending = 0;
        return false;
    }
    // reset interrupt, preverse flag
    __button_interrupt_pending &= ~_BV(0);
    return (bool)(result & _BV(0));
}

// zwraca true, jeżeli przycisk jest naciśnięty
static inline bool is_button_pressed() {
    return (bool)(__button_status.curr_status);
}

static inline void ignore_next_button_release() {
    __button_interrupt_pending |= _BV(1);
}

#endif /* PIN_IO_H_ */