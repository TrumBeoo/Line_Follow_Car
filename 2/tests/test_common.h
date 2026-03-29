// ============================================================================
// FILE: test_common.h
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: Common definitions and utilities for all tests
// ============================================================================

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "../main.h"

// ============================================================================
// GLOBAL TEST VARIABLES
// ============================================================================
extern volatile uint32_t ms_tick;
extern volatile bool run_flag;
extern volatile bool stop_flag;

// ============================================================================
// COMMON INITIALIZATION FUNCTIONS
// ============================================================================

void test_gpio_init(void) {
    ADCON1 = 0x0F;
    
    output_float(SENSOR_LEFT);
    output_float(SENSOR_MIDDLE);
    output_float(SENSOR_RIGHT);
    
    output_low(AIN1_PIN);
    output_low(BIN1_PIN);
    output_low(STBY_PIN);
    
    output_low(TRIG_PIN);
    output_float(ECHO_PIN);
    
    output_low(LED_STATUS);
    output_low(LED_ACTION);
    
    port_b_pullups(0b10001000);
}

void test_timer_init(void) {
    setup_timer_0(RTCC_INTERNAL | RTCC_DIV_256);
    set_timer0(65497);
    enable_interrupts(INT_TIMER0);
    
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_1);
    set_timer1(0);
    
    enable_interrupts(GLOBAL);
}

void test_pwm_init(void) {
    setup_ccp1(CCP_PWM);
    setup_ccp2(CCP_PWM);
    setup_timer_2(T2_DIV_BY_1, 249, 1);
}

// ============================================================================
// TIMER0 ISR
// ============================================================================
#INT_TIMER0
void timer0_isr(void) {
    set_timer0(65497);
    ms_tick += 2;
}

#endif // TEST_COMMON_H
