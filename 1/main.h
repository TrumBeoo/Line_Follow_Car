////////////////////////////////////////////////////////////////////////////////
// FILE: main.h
// DESC: Main header file with standard libraries and global definitions
////////////////////////////////////////////////////////////////////////////////

#ifndef MAIN_H
#define MAIN_H

// Include hardware configuration first
#include "config.h"

// Include subsystem headers
#include "fsm.h"
#include "motor.h"
#include "sensors.h"
#include "ultrasonic.h"

// Standard includes
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// GLOBAL TIMING VARIABLES (volatile - modified in ISR)
// ============================================================================
extern volatile int32 ms_tick;        // System millisecond counter
extern volatile int16 led_tick;       // LED blink counter

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// System initialization
void system_init(void);
void gpio_init(void);
void timers_init(void);
void pwm_init(void);
void interrupts_init(void);

// Peripheral control
void peripherals_grab_ball(void);
void peripherals_release_ball(void);

// Utility functions
void delay_ms_nonblocking(int16 ms);
int1 check_timeout(int32 start_time, int32 timeout_ms);
int32 get_ms_tick(void);

#endif // MAIN_H
