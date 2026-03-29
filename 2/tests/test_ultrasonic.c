// ============================================================================
// FILE: test_ultrasonic.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: Ultrasonic sensor test - Tests HC-SR04 distance measurement
// ============================================================================

#include "test_common.h"

volatile uint32_t ms_tick = 0;
volatile bool run_flag = false;
volatile bool stop_flag = false;

void main(void) {
    test_gpio_init();
    test_timer_init();
    
    delay_ms(1000);
    
    // Test for 60 seconds
    uint32_t start_time = ms_tick;
    uint32_t last_measure = 0;
    
    while((ms_tick - start_time) < 60000) {
        if((ms_tick - last_measure) >= 500) {
            uint8_t distance = hcsr04_read_cm();
            
            // Visual feedback based on distance
            if(distance == TIMEOUT_CM) {
                // No object - both LEDs off
                led_status_off();
                led_action_off();
            } else if(distance <= STOP_CM_STATION) {
                // Very close (<3cm) - both LEDs on
                led_status_on();
                led_action_on();
            } else if(distance <= STOP_CM_END) {
                // Close (<5cm) - status LED on
                led_status_on();
                led_action_off();
            } else if(distance <= 10) {
                // Medium distance (<10cm) - action LED on
                led_status_off();
                led_action_on();
            } else {
                // Far - both off
                led_status_off();
                led_action_off();
            }
            
            last_measure = ms_tick;
        }
    }
    
    // Success indicator
    while(true) {
        led_status_toggle();
        delay_ms(1000);
    }
}
