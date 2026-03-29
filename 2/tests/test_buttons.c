// ============================================================================
// FILE: test_buttons.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: Button test - Tests RUN and STOP buttons with debounce
// ============================================================================

#include "test_common.h"

volatile uint32_t ms_tick = 0;
volatile bool run_flag = false;
volatile bool stop_flag = false;

void main(void) {
    test_gpio_init();
    test_timer_init();
    
    delay_ms(1000);
    
    // Initial LED state
    led_status_off();
    led_action_off();
    
    bool last_run = false;
    bool last_stop = false;
    uint32_t last_run_time = 0;
    uint32_t last_stop_time = 0;
    
    // Test for 60 seconds
    uint32_t start_time = ms_tick;
    
    while((ms_tick - start_time) < 60000) {
        bool run_pressed = !input(BTN_RUN);
        bool stop_pressed = !input(BTN_STOP);
        
        // RUN button with debounce
        if(run_pressed && !last_run) {
            if((ms_tick - last_run_time) > DEBOUNCE_MS) {
                led_status_toggle();
                last_run_time = ms_tick;
            }
        }
        
        // STOP button with debounce
        if(stop_pressed && !last_stop) {
            if((ms_tick - last_stop_time) > DEBOUNCE_MS) {
                led_action_toggle();
                last_stop_time = ms_tick;
            }
        }
        
        last_run = run_pressed;
        last_stop = stop_pressed;
        
        delay_ms(10);
    }
    
    // Success indicator
    led_status_off();
    led_action_off();
    
    while(true) {
        led_status_toggle();
        led_action_toggle();
        delay_ms(500);
    }
}
