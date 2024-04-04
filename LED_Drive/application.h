/*
 * application.h
 *
 * Created: 22.03.2024 15:46:26
 *  Author: Bogusław
 */


#ifndef APPLICATION_H_
#define APPLICATION_H_

typedef struct __app_debug_status {
    uint8_t adc_voltage_hi;     // 1
                                //      (7:4) bulb,
                                //      (3:0) accu
    uint8_t bulb_voltage_lo;    // 2
    uint8_t accu_voltage_lo;    // 3
    uint8_t app_status;         // 4
                                //   • — •   0 - initial status, bulb off
                                //   •       1 - force off by user
                                //   • —     2 - transition between off and on -> to 3
                                //   — —     3 - automatic on; bulb at proper power (13,2 V)
                                //   — •     4 - still on, but waiting for being off -> to 5
                                //   • •     5 - automatic off, subject to auto on
                                //   • • —   6 - transition between off and on -> to 7
                                //   — — —   7 - force on; bulb at proper power (13,2 V)
                                //   • • •   8 - forced on, but starter running

    uint8_t input_state;        // 5
                                //   hi:
                                //      (3)   - btn - 1 - pressed
                                //      (2)   - oil - 1 - pressure
                                //      (1)   - gear enaged
                                //      (0)
                                //   lo:
                                //      (3)
                                //      (2)   - oil or charging
                                //      (1,0) - accu state
                                //              11 - charging
                                //              10 - normal
                                //              01 - starter running
                                //              00 - unknown
    uint8_t bulb_pwm;           // 6
    uint16_t adc_count;         // 7,8  - 6314 (18AA) - 6322 (18B2) - 6336 (18C0)
} app_debug_status_t;

extern app_debug_status_t app_debug_status;

// perform periodic application logic
extern void loop_application_logic();

// setup application state
extern void init_application();

#endif /* APPLICATION_H_ */