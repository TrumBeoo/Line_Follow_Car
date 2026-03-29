// ============================================================================
// FILE: test_motor.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: Motor control test - Tests all motor functions
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
    
    // Enable motor driver
    motor_enable();
    delay_ms(500);
    
    // Test 1: Forward
    led_status_on();
    motor_forward(BASE_PWM);
    delay_ms(2000);
    motor_stop();
    delay_ms(1000);
    
    // Test 2: Reverse
    motor_reverse(BACK_PWM);
    delay_ms(2000);
    motor_stop();
    delay_ms(1000);
    
    // Test 3: Turn left
    motor_turn_left(TURN_PWM, BASE_PWM);
    delay_ms(2000);
    motor_stop();
    delay_ms(1000);
    
    // Test 4: Turn right
    motor_turn_right(BASE_PWM, TURN_PWM);
    delay_ms(2000);
    motor_stop();
    delay_ms(1000);
    
    // Test 5: Pivot left
    motor_pivot_left(TURN_PWM);
    delay_ms(1500);
    motor_stop();
    delay_ms(1000);
    
    // Test 6: Pivot right
    motor_pivot_right(TURN_PWM);
    delay_ms(1500);
    motor_stop();
    delay_ms(1000);
    
    motor_disable();
    led_status_off();
    
    // Success indicator
    while(true) {
        led_action_toggle();
        delay_ms(500);
    }
}
