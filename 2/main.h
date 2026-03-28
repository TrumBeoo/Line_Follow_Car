// ============================================================================
// FILE: main.h
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: Main header file with standard library includes
// ============================================================================

#ifndef MAIN_H
#define MAIN_H

// Standard C libraries
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Project configuration
#include "config.h"

// Module headers
#include "fsm.h"
#include "motor.h"
#include "sensors.h"
#include "ultrasonic.h"

// ============================================================================
// GLOBAL VARIABLES (declared in main.c)
// ============================================================================

// System timing
extern volatile uint32_t ms_tick;           // System millisecond counter
extern volatile uint32_t led_tick;          // LED blink counter

// State machine
extern volatile SystemState_t current_state; // Current FSM state
extern volatile SystemState_t next_state;    // Next state to transition to

// Control flags
extern volatile bool run_flag;              // Run button pressed flag
extern volatile bool stop_flag;             // Stop button pressed flag  
extern volatile bool ball_ok;               // Ball detected flag

// Sensor data
extern volatile uint8_t sensor_pattern;     // Current sensor reading
extern volatile uint8_t last_valid_pattern; // Last valid pattern before line loss

// Checkpoint management  
extern volatile uint8_t checkpoint_state;   // Current checkpoint (0-2)

// Error handling
extern volatile uint8_t error_count;        // Error retry counter
extern volatile ErrorType_t error_type;     // Type of error encountered

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// Initialization
void system_init(void);
void gpio_init(void);
void timer_init(void);
void pwm_init(void);
void interrupt_init(void);

// Interrupt Service Routines
void timer0_isr(void);
void iocb_isr(void);

// Utility functions
void delay_us(uint16_t us);
void delay_ms(uint16_t ms);

#endif // MAIN_H