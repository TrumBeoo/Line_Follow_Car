////////////////////////////////////////////////////////////////////////////////
// FILE: motor.c
// DESC: Motor control implementation with PWM and lookup table
// PWM: Hardware PWM (CCP1/RC2) for left motor
//      Software PWM (Timer2 interrupt) for right motor
////////////////////////////////////////////////////////////////////////////////

#include "main.h"

// ============================================================================
// SOFTWARE PWM VARIABLES (for right motor on RC3)
// ============================================================================
volatile int8 duty_R = 0;          // Right motor duty cycle (0-255)
volatile int8 timer2_count = 0;    // Timer2 counter for PWM comparison

// ============================================================================
// LOOKUP TABLE (stored in ROM to save RAM)
// Maps sensor patterns (0b000 to 0b111) to motor commands
// Pattern format: [Left][Mid][Right] where 1=black line, 0=white surface
// ============================================================================
const MotorCmd_t motor_table[8] = {
    // 0b000: All white (line lost) - maintain previous direction
    {BASE_PWM, BASE_PWM, DIR_FORWARD},
    
    // 0b001: Strong right deviation - turn right strongly
    {BASE_PWM, SLOW_PWM, DIR_FORWARD},
    
    // 0b010: Centered on line - go straight
    {BASE_PWM, BASE_PWM, DIR_FORWARD},
    
    // 0b011: Light right deviation - turn right gently
    {BASE_PWM, BASE_PWM - 30, DIR_FORWARD},
    
    // 0b100: Strong left deviation - turn left strongly
    {SLOW_PWM, BASE_PWM, DIR_FORWARD},
    
    // 0b101: T-junction or end point - stop
    {STOP_PWM, STOP_PWM, DIR_FORWARD},
    
    // 0b110: Light left deviation - turn left gently
    {BASE_PWM - 30, BASE_PWM, DIR_FORWARD},
    
    // 0b111: Intersection - go straight (will be handled by FSM)
    {BASE_PWM, BASE_PWM, DIR_FORWARD}
};

// ============================================================================
// TIMER2 INTERRUPT - SOFTWARE PWM FOR RIGHT MOTOR
// Triggers at 20kHz (every 50µs)
// ============================================================================
#INT_TIMER2
void timer2_isr(void) {
    // Compare duty_R with timer2_count for PWM output
    if (timer2_count < duty_R) {
        output_high(MOTOR_PWMB);  // RC3 HIGH during ON phase
    } else {
        output_low(MOTOR_PWMB);   // RC3 LOW during OFF phase
    }
    
    // Increment counter and wrap at 256 (matches 8-bit duty cycle)
    timer2_count++;
}

// ============================================================================
// MOTOR INITIALIZATION
// ============================================================================
void motor_init(void) {
    // Setup Timer2 for 20kHz PWM frequency
    // Fosc = 20MHz, PWM freq = Fosc / (4 * (PR2+1) * TMR2 prescaler)
    // For 20kHz: PR2 = 249 with prescaler = 1
    setup_timer_2(T2_DIV_BY_1, 249, 1);
    
    // Setup CCP1 as hardware PWM (MOTOR_PWMA - Left motor on RC2)
    setup_ccp1(CCP_PWM);
    
    // RC3 (MOTOR_PWMB) configured as digital output for software PWM
    // (CCP2 disabled to avoid conflict)
    
    // Initialize direction pins
    output_low(MOTOR_AIN1);
    output_low(MOTOR_BIN1);
    
    // Enable motor driver (STBY pin high)
    output_high(MOTOR_STBY);
    
    // Enable Timer2 interrupt for software PWM
    enable_interrupts(INT_TIMER2);
    
    // Start with motors stopped
    motor_stop();
}

// ============================================================================
// SET MOTOR SPEEDS AND DIRECTION
// ============================================================================
void motor_set(int8 left_pwm, int8 right_pwm, int1 direction) {
    // Set direction
    if (direction == DIR_FORWARD) {
        output_low(MOTOR_AIN1);   // AIN1=0, AIN2=1 (from driver logic)
        output_low(MOTOR_BIN1);   // BIN1=0, BIN2=1
    } else {
        output_high(MOTOR_AIN1);  // AIN1=1, AIN2=0 (reverse)
        output_high(MOTOR_BIN1);  // BIN1=1, BIN2=0
    }
    
    // Set left motor PWM (hardware PWM on CCP1)
    // CCS uses 10-bit resolution: scale 8-bit to 10-bit
    set_pwm1_duty((int16)left_pwm * 4);
    
    // Set right motor PWM (software PWM via Timer2 interrupt)
    // Update duty_R directly (0-255 range)
    duty_R = right_pwm;
}

// ============================================================================
// GET MOTOR COMMAND FROM LOOKUP TABLE
// ============================================================================
MotorCmd_t motor_get_command(int8 pattern) {
    // Ensure pattern is within valid range (0-7)
    pattern &= 0x07;
    return motor_table[pattern];
}

// ============================================================================
// MOTOR STOP
// ============================================================================
void motor_stop(void) {
    motor_set(STOP_PWM, STOP_PWM, DIR_FORWARD);
}

// ============================================================================
// MOTOR FORWARD
// ============================================================================
void motor_forward(int8 speed) {
    motor_set(speed, speed, DIR_FORWARD);
}

// ============================================================================
// MOTOR REVERSE
// ============================================================================
void motor_reverse(int8 speed) {
    motor_set(speed, speed, DIR_REVERSE);
}

// ============================================================================
// TURN RIGHT (while moving forward)
// ============================================================================
void motor_turn_right(int8 speed) {
    motor_set(speed, speed/2, DIR_FORWARD);
}

// ============================================================================
// TURN LEFT (while moving forward)
// ============================================================================
void motor_turn_left(int8 speed) {
    motor_set(speed/2, speed, DIR_FORWARD);
}

// ============================================================================
// PIVOT RIGHT (turn in place)
// ============================================================================
void motor_pivot_right(int8 speed) {
    // Left forward, right reverse
    set_pwm1_duty((int16)speed * 4);
    duty_R = speed;
    output_low(MOTOR_AIN1);    // Left forward
    output_high(MOTOR_BIN1);   // Right reverse
}

// ============================================================================
// PIVOT LEFT (turn in place)
// ============================================================================
void motor_pivot_left(int8 speed) {
    // Left reverse, right forward
    set_pwm1_duty((int16)speed * 4);
    duty_R = speed;
    output_high(MOTOR_AIN1);   // Left reverse
    output_low(MOTOR_BIN1);    // Right forward
}

// ============================================================================
// ENABLE MOTOR DRIVER
// ============================================================================
void motor_enable(void) {
    output_high(MOTOR_STBY);
}

// ============================================================================
// DISABLE MOTOR DRIVER
// ============================================================================
void motor_disable(void) {
    output_low(MOTOR_STBY);
    motor_stop();
}

