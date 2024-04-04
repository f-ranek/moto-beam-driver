/*
 * spi.c
 *
 * Created: 09.03.2024 23:02:21
 *  Author: Bogusław
 */
#include <avr/io.h>
#include "spi.h"

static inline uint8_t swap_nibbles(uint8_t byte) {
    return (byte >> 4) | (byte << 4);
}

__attribute__((noinline)) static void emmit_spi_byte(uint8_t byte) {

    // DATA - PA4 - LSB to MSB
    // CLK  - PA5

    const uint8_t port_template = swap_nibbles(PORTA & ~(_BV(5) | _BV(4)));
    for (uint8_t i=0; i<8; i++){
        uint8_t port = (byte & 0x01) | port_template;
        // clear CLK and store DATA
        PORTA = swap_nibbles(port);
        // set CLK
        PORTA |= _BV(5);
        byte >>= 1;
    }
    __asm__ __volatile__ (
        "nop" "\n"
        "nop" "\n"
        "nop" "\n"
        "nop" "\n"
        "nop" "\n"
    );
    PORTA = swap_nibbles(port_template);
}

void emmit_spi_data(void* data, uint8_t size) {
    uint8_t* data2 = (uint8_t*)data;
    while(size != 0) {
        emmit_spi_byte(*data2);
        data2++;
        size--;
    }
}
