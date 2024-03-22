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
    uint8_t app_status;         // 4
    uint8_t bulb_pwm;           // 5
    uint16_t adc_count;         // 6,7  - 6314 (18AA) - 6322 (18B2) - 6336 (18C0)
} app_debug_status_t;

extern app_debug_status_t app_debug_status;

// perform periodic application logic
extern void loop_application_logic();

// setup application state
extern void init_application();

#endif /* APPLICATION_H_ */