// ============================================================================
// FILE: test_pwm.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: PWM test - Tests different duty cycles on CCP1 and CCP2
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
    output_high(AIN1_PIN);
    output_high(BIN1_PIN);
    
    // Test different duty cycles: 0%, 25%, 50%, 63%, 75%, 100%
    uint8_t duties[] = {0, 64, 128, 160, 192, 255};
    
    for(uint8_t i = 0; i < 6; i++) {
        // Indicate duty level with LED blinks
        led_status_off();
        led_action_off();
        delay_ms(500);
        
        for(uint8_t j = 0; j <= i; j++) {
            led_status_on();
            delay_ms(150);
            led_status_off();
            delay_ms(150);
        }
        
        delay_ms(500);
        
        // Apply PWM
        led_action_on();
        set_pwm1_duty(duties[i]);
        set_pwm2_duty(duties[i]);
        delay_ms(3000);
        led_action_off();
        delay_ms(500);
    }
    
    set_pwm1_duty(0);
    set_pwm2_duty(0);
    motor_disable();
    
    // Success indicator
    while(true) {
        led_status_toggle();
        delay_ms(200);
    }
}
