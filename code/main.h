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
extern volatile int1 led2_blink_enable;  // Enable LED2 blinking
extern volatile int8 led2_blink_count;   // Count LED2 blinks
extern volatile int1 programming_mode;   // Programming mode flag
extern volatile int16 led1_blink_tick;   // LED1 blink counter for programming mode

// ============================================================================
// GLOBAL BUTTON FLAGS (volatile - modified in ISR)
// ============================================================================
extern volatile int1 system_enabled;     // System ON/OFF state (RB7)
extern volatile int1 stop_pressed;       // STOP button - checkpoint logic (RB0)
extern volatile int1 run_pressed;        // RUN button - start from checkpoint (RB1)
extern volatile int1 cp1_long_press;     // Flag nh?n gi? STOP

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

// Programming mode control
void enter_programming_mode(void);
void exit_programming_mode(void);
int1 is_programming_mode(void);

// Checkpoint button handling
void handle_checkpoint_buttons(void);

// Utility functions
void delay_ms_nonblocking(int16 ms);
int1 check_timeout(int32 start_time, int32 timeout_ms);
int32 get_ms_tick(void);

#endif // MAIN_H

