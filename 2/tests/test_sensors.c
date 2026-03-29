// ============================================================================
// FILE: test_sensors.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: IR sensor test - Tests sensor reading and chatter filter
// ============================================================================

#include "test_common.h"

volatile uint32_t ms_tick = 0;
volatile bool run_flag = false;
volatile bool stop_flag = false;

void main(void) {
    test_gpio_init();
    test_timer_init();
    
    delay_ms(1000);
    
    // Calibrate sensors
    sensor_calibrate();
    
    // Test for 60 seconds
    uint32_t start_time = ms_tick;
    
    while((ms_tick - start_time) < 60000) {
        uint8_t raw = read_sensor_pattern();
        uint8_t filtered = chatter_filter(raw);
        
        // Visual feedback on LEDs
        // Status LED: Shows if on center
        if(filtered == PATTERN_CENTER) {
            led_status_on();
        } else {
            led_status_off();
        }
        
        // Action LED: Shows if line detected
        if(filtered != PATTERN_LOST) {
            led_action_on();
        } else {
            led_action_off();
        }
        
        delay_ms(100);
    }
    
    // Success indicator
    led_status_off();
    led_action_off();
    
    while(true) {
        led_status_toggle();
        led_action_toggle();
        delay_ms(200);
    }
}
