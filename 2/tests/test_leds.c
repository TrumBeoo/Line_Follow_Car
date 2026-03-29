// ============================================================================
// FILE: test_leds.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: LED test - Tests all LED functions
// ============================================================================

#include "test_common.h"

volatile uint32_t ms_tick = 0;
volatile bool run_flag = false;
volatile bool stop_flag = false;

void main(void) {
    test_gpio_init();
    test_timer_init();
    
    delay_ms(1000);
    
    // Test 1: Alternating blink (10 times)
    for(uint8_t i = 0; i < 10; i++) {
        led_status_on();
        led_action_off();
        delay_ms(300);
        led_status_off();
        led_action_on();
        delay_ms(300);
    }
    led_action_off();
    delay_ms(1000);
    
    // Test 2: Simultaneous blink (10 times)
    for(uint8_t i = 0; i < 10; i++) {
        led_status_on();
        led_action_on();
        delay_ms(300);
        led_status_off();
        led_action_off();
        delay_ms(300);
    }
    delay_ms(1000);
    
    // Test 3: Fast toggle (20 times)
    for(uint8_t i = 0; i < 20; i++) {
        led_status_toggle();
        led_action_toggle();
        delay_ms(100);
    }
    
    led_status_off();
    led_action_off();
    delay_ms(1000);
    
    // Success indicator - slow blink
    while(true) {
        led_status_toggle();
        led_action_toggle();
        delay_ms(1000);
    }
}
