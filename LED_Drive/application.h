/*
 * application.h
 *
 * Created: 22.03.2024 15:46:26
 *  Author: Bogusław
 */


#ifndef APPLICATION_H_
#define APPLICATION_H_

typedef struct __app_debug_status {
    uint8_t adc_voltage_hi;     // bulb, accu - HI 4 bits
    uint8_t bulb_voltage_lo;
    uint8_t accu_voltage_lo;
    uint8_t bulb_pwm;           // 4
    uint8_t app_status;         // 5
    uint16_t adc_count;         // 6,7  - 6480 - 6503 ???
} app_debug_status_t;

extern app_debug_status_t app_debug_status;


#endif /* APPLICATION_H_ */