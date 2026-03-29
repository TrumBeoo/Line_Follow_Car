// ============================================================================
// FILE: test_lookup_table.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: Lookup table test - Tests all 8 sensor patterns
// ============================================================================

#include "test_common.h"

volatile uint32_t ms_tick = 0;
volatile bool run_flag = false;
volatile bool stop_flag = false;

void main(void) {
    test_gpio_init();
    test_timer_init();
    test_pwm_init();
    
    delay_ms(1000);
    
    motor_enable();
    
    // Test all 8 patterns sequentially
    for(uint8_t pattern = 0; pattern < 8; pattern++) {
        // Indicate pattern number with LED blinks
        led_status_off();
        delay_ms(500);
        
        for(uint8_t i = 0; i <= pattern; i++) {
            led_status_on();
            delay_ms(200);
            led_status_off();
            delay_ms(200);
        }
        
        delay_ms(500);
        
        // Apply pattern
        led_action_on();
        motor_apply_table(pattern);
        delay_ms(3000);
        motor_stop();
        led_action_off();
        delay_ms(1000);
    }
    
    motor_disable();
    
    // Success indicator - fast blink
    while(true) {
        led_status_toggle();
        led_action_toggle();
        delay_ms(100);
    }
}
