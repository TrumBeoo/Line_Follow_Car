// ============================================================================
// FILE: motor.h
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: Motor control and lookup table declarations
// ============================================================================

#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// ============================================================================
// MOTOR COMMAND STRUCTURE
// ============================================================================

typedef struct {
    uint8_t left_speed;     // Left motor PWM (0-255)
    uint8_t right_speed;    // Right motor PWM (0-255)
    bool forward;           // true = forward, false = reverse
} MotorCmd_t;

// ============================================================================
// MOTOR DIRECTION ENUMERATION
// ============================================================================

typedef enum {
    DIR_FORWARD = 0,
    DIR_REVERSE,
    DIR_PIVOT_LEFT,         // Pivot turn left (left reverse, right forward)
    DIR_PIVOT_RIGHT         // Pivot turn right (left forward, right reverse)
} MotorDir_t;

// ============================================================================
// LOOKUP TABLE (stored in ROM/FLASH)
// ============================================================================

// Motor lookup table: maps 3-bit sensor pattern to motor command
// Pattern bits: [LEFT][MIDDLE][RIGHT]
extern const MotorCmd_t motor_table[8];

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// Set motor speeds and direction
void motor_set(uint8_t left_pwm, uint8_t right_pwm, MotorDir_t direction);

// Convenience functions
void motor_stop(void);
void motor_forward(uint8_t speed);
void motor_reverse(uint8_t speed);
void motor_turn_left(uint8_t left_speed, uint8_t right_speed);
void motor_turn_right(uint8_t left_speed, uint8_t right_speed);
void motor_pivot_left(uint8_t speed);
void motor_pivot_right(uint8_t speed);

// Apply lookup table command
void motor_apply_table(uint8_t pattern);

// Enable/disable motor driver
void motor_enable(void);
void motor_disable(void);

#endif // MOTOR_H