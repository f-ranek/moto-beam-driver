/*
 * spi.c
 *
 * Created: 09.03.2024 23:02:21
 *  Author: Bogusław
 */
#include <avr/io.h>
#include "spi.h"

static inline void emmit_spi_byte(uint8_t byte) {
    USIDR = byte;
    const uint8_t low  = _BV(USIWM0)|_BV(USITC);
    const uint8_t high = _BV(USIWM0)|_BV(USITC)|_BV(USICLK);
    // 8 times
    __asm__ __volatile__ (
    "out %[USICR_], %[low_]"            "\n"
    "out %[USICR_], %[high_]"           "\n"
    "out %[USICR_], %[low_]"            "\n"
    "out %[USICR_], %[high_]"           "\n"
    "out %[USICR_], %[low_]"            "\n"
    "out %[USICR_], %[high_]"           "\n"
    "out %[USICR_], %[low_]"            "\n"
    "out %[USICR_], %[high_]"           "\n"
    "out %[USICR_], %[low_]"            "\n"
    "out %[USICR_], %[high_]"           "\n"
    "out %[USICR_], %[low_]"            "\n"
    "out %[USICR_], %[high_]"           "\n"
    "out %[USICR_], %[low_]"            "\n"
    "out %[USICR_], %[high_]"           "\n"
    "out %[USICR_], %[low_]"            "\n"
    "out %[USICR_], %[high_]"           "\n"
    :
    : [low_]              "r" (low),
    [high_]               "r" (high),
    [USICR_]              "I" (_SFR_IO_ADDR(USICR))
    );
    USICR = 0;
}

void emmit_spi_data(void* data, uint8_t size) {
    uint8_t* data2 = (uint8_t*)data;
    while(size != 0) {
        emmit_spi_byte(*data2);
        data2++;
        size--;
    }
}
