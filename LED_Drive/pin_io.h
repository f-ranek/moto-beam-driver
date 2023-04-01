/*
 * pin_io.h
 *
 * Created: 27.03.2023 22:40:54
 *  Author: Bogusław
 */


#ifndef PIN_IO_H_
#define PIN_IO_H_

#include <stdbool.h>

// read stable pin values
extern inline void read_pin_values();

typedef struct __pin_status_s {
    uint8_t temp_status_since;
    uint8_t curr_status;
    uint8_t temp_status;
} __pin_status;

extern __pin_status __button_status;
extern __pin_status __ignition_starter_status;
extern __pin_status __neutral_status;
extern uint8_t __button_interrupt_pending;

// zwraca dwa bity opisujące stan wejść zapłonu oraz rozrusznika
static inline uint8_t get_ignition_starter_status() {
    return __ignition_starter_status.curr_status;
}

// zwraca jeden bit opisujący stan wejścia biegu neutralnego
// 0 -> bieg neutralny
// 1 -> dowolny inny bieg
static inline bool is_gear_engaged() {
    return __neutral_status.curr_status;
}

// zwraca oraz resetuje flagę informującą o zwolnieniu przycisku
static inline bool exchange_button_release_flag() {
    uint8_t result = __button_interrupt_pending;
    switch (result) {
        case 0:
            break;
        case 1:
            __button_interrupt_pending = 0;
            break;
        case 2:
            // no interrupt, but flag is set
            break;
        case 3:
            // flag is set, reset interrupt status as well as flag
            __button_interrupt_pending = 0;
            result = 0;
    }
    return (bool)(result&1);
}

// zwraca 1, jeżeli przycisk jest naciśnięty
static inline bool is_button_pressed() {
    return __button_status.curr_status == 0 ? true : false;
}

static inline void ignore_next_button_release() {
    __button_interrupt_pending |= 2;
}

#endif /* PIN_IO_H_ */