// ============================================================================
// FILE: test_timer.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: Timer test - Tests Timer0 2ms interrupt accuracy
// ============================================================================

#include "test_common.h"

volatile uint32_t ms_tick = 0;
volatile bool run_flag = false;
volatile bool stop_flag = false;

void main(void) {
    test_gpio_init();
    test_timer_init();
    
    delay_ms(1000);
    
    // Test Timer0 accuracy for 60 seconds
    // LED should blink every 500ms (250 blinks total)
    uint32_t last_blink = 0;
    uint32_t start_time = ms_tick;
    uint16_t blink_count = 0;
    
    while((ms_tick - start_time) < 60000) {
        if((ms_tick - last_blink) >= 500) {
            led_status_toggle();
            last_blink = ms_tick;
            blink_count++;
            
            // Action LED shows tens digit
            if((blink_count % 10) == 0) {
                led_action_toggle();
            }
        }
    }
    
    // Should have ~120 blinks (60s / 0.5s)
    // If close to 120, timer is accurate
    
    // Success indicator - show result
    led_status_off();
    led_action_off();
    delay_ms(1000);
    
    // Blink count / 10 times on status LED
    for(uint8_t i = 0; i < (blink_count / 10); i++) {
        led_status_on();
        delay_ms(200);
        led_status_off();
        delay_ms(200);
    }
    
    delay_ms(2000);
    
    // Continuous blink
    while(true) {
        led_status_toggle();
        led_action_toggle();
        delay_ms(500);
    }
}
