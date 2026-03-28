// ============================================================================
// FILE: motor.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: Motor control implementation with lookup table
// ============================================================================

#include "motor.h"
#include "main.h"

// ============================================================================
// MOTOR LOOKUP TABLE (stored in ROM for RAM optimization)
// ============================================================================
// Pattern format: [LEFT][MIDDLE][RIGHT] (3-bit)
// 0 = white surface, 1 = black line (after inversion)

const MotorCmd_t motor_table[8] = {
    // Pattern 0b000: Line lost - maintain last direction
    {STOP_PWM, STOP_PWM, true},
    
    // Pattern 0b001: Hard right turn
    {BASE_PWM, TURN_PWM, true},
    
    // Pattern 0b010: Center - go straight
    {BASE_PWM, BASE_PWM, true},
    
    // Pattern 0b011: Slight right adjustment
    {BASE_PWM, SLOW_PWM, true},
    
    // Pattern 0b100: Hard left turn
    {TURN_PWM, BASE_PWM, true},
    
    // Pattern 0b101: T-junction (treat as intersection)
    {BASE_PWM, BASE_PWM, true},
    
    // Pattern 0b110: Slight left adjustment
    {SLOW_PWM, BASE_PWM, true},
    
    // Pattern 0b111: Intersection - go straight
    {BASE_PWM, BASE_PWM, true}
};

// ============================================================================
// MOTOR CONTROL FUNCTIONS
// ============================================================================

/**
 * Set motor speeds and direction
 * @param left_pwm: Left motor PWM duty cycle (0-255)
 * @param right_pwm: Right motor PWM duty cycle (0-255)
 * @param direction: Motor direction (forward/reverse/pivot)
 */
void motor_set(uint8_t left_pwm, uint8_t right_pwm, MotorDir_t direction) {
    // Set PWM duty cycles
    set_pwm1_duty(left_pwm);   // CCP1 - Left motor (RC2)
    set_pwm2_duty(right_pwm);  // CCP2 - Right motor (RC3, but physically on C1)
    
    // Set direction pins based on direction mode
    switch(direction) {
        case DIR_FORWARD:
            output_high(AIN1_PIN);  // Left forward
            output_high(BIN1_PIN);  // Right forward
            break;
            
        case DIR_REVERSE:
            output_low(AIN1_PIN);   // Left reverse
            output_low(BIN1_PIN);   // Right reverse
            break;
            
        case DIR_PIVOT_LEFT:
            output_low(AIN1_PIN);   // Left reverse
            output_high(BIN1_PIN);  // Right forward
            break;
            
        case DIR_PIVOT_RIGHT:
            output_high(AIN1_PIN);  // Left forward
            output_low(BIN1_PIN);   // Right reverse
            break;
    }
}

/**
 * Stop both motors
 */
void motor_stop(void) {
    motor_set(STOP_PWM, STOP_PWM, DIR_FORWARD);
}

/**
 * Move forward at specified speed
 */
void motor_forward(uint8_t speed) {
    motor_set(speed, speed, DIR_FORWARD);
}

/**
 * Move reverse at specified speed
 */
void motor_reverse(uint8_t speed) {
    motor_set(speed, speed, DIR_REVERSE);
}

/**
 * Turn left (reduce left motor speed)
 */
void motor_turn_left(uint8_t left_speed, uint8_t right_speed) {
    motor_set(left_speed, right_speed, DIR_FORWARD);
}

/**
 * Turn right (reduce right motor speed)
 */
void motor_turn_right(uint8_t left_speed, uint8_t right_speed) {
    motor_set(left_speed, right_speed, DIR_FORWARD);
}

/**
 * Pivot turn left (left reverse, right forward)
 */
void motor_pivot_left(uint8_t speed) {
    motor_set(speed, speed, DIR_PIVOT_LEFT);
}

/**
 * Pivot turn right (left forward, right reverse)
 */
void motor_pivot_right(uint8_t speed) {
    motor_set(speed, speed, DIR_PIVOT_RIGHT);
}

/**
 * Apply motor command from lookup table
 */
void motor_apply_table(uint8_t pattern) {
    const MotorCmd_t *cmd = &motor_table[pattern & 0x07];
    
    if(cmd->forward) {
        motor_set(cmd->left_speed, cmd->right_speed, DIR_FORWARD);
    } else {
        motor_set(cmd->left_speed, cmd->right_speed, DIR_REVERSE);
    }
}

/**
 * Enable motor driver (STBY pin HIGH)
 */
void motor_enable(void) {
    output_high(STBY_PIN);
}

/**
 * Disable motor driver (STBY pin LOW)
 */
void motor_disable(void) {
    output_low(STBY_PIN);
    motor_stop();
}