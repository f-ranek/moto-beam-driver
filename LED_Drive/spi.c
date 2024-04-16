/*
 * spi.c
 *
 * Created: 09.03.2024 23:02:21
 *  Author: Bogusław
 */
#include <avr/io.h>
#include <util/atomic.h>
#include "spi.h"

__attribute__((noinline)) static void emmit_spi_byte(uint8_t byte) {

    // DATA - PA4 - MSB to LSB
    // CLK  - PA5
    uint8_t tmp;

    __asm__ __volatile__ (
       // load PORTA to temp storage
       "in %[tmp_], %[PORTA_]"  "\n"
       // clean bits 4 (DATA) and 5 (CLK)
       "andi %[tmp_], 0xCF"     "\n"
       : [tmp_]                "=d" (tmp)
       : [PORTA_]              "I" (_SFR_IO_ADDR(PORTA))
    );

#define EMMIT_BIT(BIT_NO)                       \
    __asm__ __volatile__ (                      \
        /* copy BIT# from data to T reg */      \
        "bst %[byte_], " #BIT_NO "\n"           \
        /* copy T to tmp_ at pos 4 (DATA) */    \
        "bld %[tmp_], 4"         "\n"           \
        /* emmit DATA and CLK = 0 */            \
        "out %[PORTA_], %[tmp_]" "\n"           \
        /* emmit CLK = 1 */                     \
        "sbi %[PORTA_], 5"           "\n"       \
        : [tmp_]              "+d" (tmp)                    \
        : [byte_]             "r" (byte),                   \
          [PORTA_]            "I" (_SFR_IO_ADDR(PORTA))     \
    )

    EMMIT_BIT(7);
    EMMIT_BIT(6);
    EMMIT_BIT(5);
    EMMIT_BIT(4);
    EMMIT_BIT(3);
    EMMIT_BIT(2);
    EMMIT_BIT(1);
    EMMIT_BIT(0);

    __asm__ __volatile__ (
        // clean bits 4 (DATA) and 5 (CLK)
        "andi %[tmp_], 0xCF"        "\n"
        // wait to make smooth timing
        "nop"                       "\n"
        // emmit PORTA with both DATA and CLK reset
        "out %[PORTA_], %[tmp_]"    "\n"
        : [tmp_]                "+d" (tmp)
        : [PORTA_]              "I" (_SFR_IO_ADDR(PORTA))
    );
}

void emmit_spi_data(void* data, uint8_t size) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        uint8_t* data2 = (uint8_t*)data;
        while(size != 0) {
            emmit_spi_byte(*data2);
            data2++;
            size--;
        }
    }
}
