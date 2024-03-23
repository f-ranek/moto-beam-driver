/*
 * morse.c
 *
 * Created: 23.03.2024 21:38:01
 *  Author: Bogusław
 */
#include <avr/pgmspace.h>

#include "timer.h"
#include "pin_io.h"

#ifndef SIMULATION

// 135 ms
#define INTERVAL_MORSE_DOT    ((uint8_t)45)
// 180 ms
#define INTERVAL_MORSE_WAIT   ((uint8_t)60)
// 450 ms
#define INTERVAL_MORSE_DASH   ((uint8_t)(150))
// 2,25 sek - tu musi być na tyle czasu, żeby zmieścić całą literę plus przerwa
#define INTERVAL_MORSE_SYM    ((uint16_t)(2250 / 3))

#else // !SIMULATION

#define INTERVAL_MORSE_DOT    ((uint8_t)1)
#define INTERVAL_MORSE_WAIT   ((uint8_t)1)
#define INTERVAL_MORSE_DASH   ((uint8_t)(3 * INTERVAL_MORSE_DOT))
#define INTERVAL_MORSE_SYM    ((uint16_t)(20))

#endif // SIMULATION

#define MAKE_MORSE_3(A,B,C) 0b00 ## C ## B ## A
#define RESOVLE_MORSE_0(A)  0b ## A

#define MAKE_MORSE(A,B,C)   MAKE_MORSE_3(A,B,C)
#define RESOVLE_MORSE(A)    RESOVLE_MORSE_0(A)

#define M_DOT      10
#define M_DASH     11
#define M_NONE     00

static const uint8_t MORSE_CODES[9] PROGMEM = {
    // APP_INIT
    // 0 - R   • — •
    MAKE_MORSE (M_DOT, M_DASH, M_DOT),

    // APP_FORCE_OFF
    // 1 - E   •
    MAKE_MORSE (M_DOT, M_NONE, M_NONE),

    // APP_AUTO_BRIGHTENING
    // 2 - A   • —
    MAKE_MORSE (M_DOT, M_DASH, M_NONE),

    // APP_AUTO_ON
    // 3 - M   — —
    MAKE_MORSE (M_DASH, M_DASH, M_NONE),

    // APP_WAITING_FOR_BULB_OFF
    // 4 - N   — •
    MAKE_MORSE (M_DASH, M_DOT, M_NONE),

    // APP_AUTO_OFF
    // 5 - I   • •
    MAKE_MORSE (M_DOT, M_DOT, M_NONE),

    // APP_FORCE_BRIGHTENING
    // 6 - U   • • —
    MAKE_MORSE (M_DOT, M_DOT, M_DASH),

    // APP_FORCE_ON
    // 7 - O   — — —
    MAKE_MORSE (M_DASH, M_DASH, M_DASH),

    // APP_FORCE_ON_STARTER
    // 8 - S   • • •
    MAKE_MORSE (M_DOT, M_DOT, M_DOT),
};

static uint16_t morse_next_emmission;
static uint16_t morse_next_flip;
static uint8_t  morse_symbol;

static inline void prepare_status_code(uint8_t status_code)
{
    const uint16_t timer = get_timer_value();
    if (morse_next_emmission != timer) {
        return ;
    }

    if (status_code < sizeof(MORSE_CODES)) {
        morse_next_flip = timer;
        morse_symbol = pgm_read_byte(&(MORSE_CODES[status_code]));
    } else {
        // nothing to emmit
        morse_symbol = 0;
    }

    morse_next_emmission = timer + INTERVAL_MORSE_SYM;
    clear_debug_bit();
}

static inline void emmit_next_symbol()
{
    const uint16_t timer = get_timer_value();
    if (morse_next_flip != timer) {
        return ;
    }

    if (is_debug_bit_set()) {
        // nowy symbol - stała przerwa
        morse_next_flip = timer + (uint16_t)INTERVAL_MORSE_WAIT;
        morse_symbol >>= 2;
        clear_debug_bit();
        return ;
    }

    const uint8_t morse_code = morse_symbol & 0x03;
    // początek symbolu
    switch(morse_code) {
        case RESOVLE_MORSE(M_DOT):
            morse_next_flip = timer + (uint16_t)INTERVAL_MORSE_DOT;
            set_debug_bit();
            break;
        case RESOVLE_MORSE(M_DASH):
            morse_next_flip = timer + (uint16_t)INTERVAL_MORSE_DASH;
            set_debug_bit();
            break;
        default:
            clear_debug_bit();
            break;
    }
}

void emmit_morse_status_code_if_ready(uint8_t status_code)
{
    prepare_status_code(status_code);
    emmit_next_symbol();
}

