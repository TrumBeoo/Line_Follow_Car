////////////////////////////////////////////////////////////////////////////////
// FILE: main.h
// DESC: Main header - type definitions, global variables, and function prototypes
//       Consolidates: fsm.h, motor.h, sensors.h, ultrasonic.h, main.h
// DEVICE: PIC18F2685 @ 20MHz
////////////////////////////////////////////////////////////////////////////////

#ifndef MAIN_H
#define MAIN_H

#include "config.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// SECTION 1: FSM - STATE & TYPE DEFINITIONS
// ============================================================================

// System states (13 states)
typedef enum {
    STATE_IDLE = 0,           // Waiting for RUN button
    STATE_FOLLOW_LINE,        // Normal line following
    STATE_STATION,            // Approaching ball pickup station
    STATE_STATION_STOP,       // Stopped at station, grabbing ball
    STATE_STATION_BACK,       // Reversing from station
    STATE_NAVIGATION,         // Turning at junction (T or intersection)
    STATE_END,                // Detected END point, stop
    STATE_END_PUSH,           // Continue pushing 300ms to hit release mechanism
    STATE_END_WAIT,           // Wait 2 seconds for ball to drop
    STATE_END_REVERSE,        // Reverse until line detected (000)
    STATE_ERROR,              // Error recovery state
    STATE_CHECKPOINT          // Checkpoint management state
} SystemState_t;

// Error types
typedef enum {
    ERR_NONE = 0,
    ERR_LOST_LINE,            // Lost line for too long
    ERR_NOISE,                // Sensor noise detected
    ERR_ULTRASONIC,           // Ultrasonic timeout
    ERR_BALL_GRAB             // Ball grab failed
} ErrorType_t;

// Navigation direction at junction
typedef enum {
    NAV_STRAIGHT = 0,
    NAV_RIGHT,
    NAV_LEFT,
    NAV_UTURN
} NavDirection_t;

// Checkpoint positions
typedef enum {
    CP_START = 1,             // Starting position (checkpoint 1)
    CP_AFTER_NAVIGATION,      // After navigation success (checkpoint 2)
    CP_BEFORE_END             // Before END point (checkpoint 3)
} Checkpoint_t;

// ============================================================================
// SECTION 2: MOTOR - TYPE DEFINITIONS
// ============================================================================

// Motor command structure (used by lookup table)
typedef struct {
    int8 left_pwm;            // Left motor PWM (0-255)
    int8 right_pwm;           // Right motor PWM (0-255)
    int1 direction;           // 1=Forward, 0=Reverse
} MotorCmd_t;

// ============================================================================
// SECTION 3: GLOBAL VARIABLES - FSM STATE
// (defined in main.c, used across all modules)
// ============================================================================
extern volatile SystemState_t current_state;
extern volatile SystemState_t previous_state;
extern ErrorType_t            current_error;
extern NavDirection_t         nav_direction;
extern Checkpoint_t           current_checkpoint;
extern volatile int1          run_flag;
extern int8                   retry_count;

// ============================================================================
// SECTION 4: GLOBAL VARIABLES - TIMING
// (volatile: modified in ISR)
// ============================================================================
extern volatile int32 ms_tick;           // System millisecond counter
extern volatile int16 led_tick;          // LED blink counter
extern volatile int1  led2_blink_enable; // Enable LED2 blinking
extern volatile int8  led2_blink_count;  // Count LED2 blinks
extern volatile int1  programming_mode;  // Programming mode flag (unused)
extern volatile int16 led1_blink_tick;   // LED1 blink counter (unused)

// ============================================================================
// SECTION 5: GLOBAL VARIABLES - BUTTON FLAGS
// (volatile: modified in ISR)
// ============================================================================
extern volatile int1 system_enabled;     // System ON/OFF state (RB7)
extern volatile int1 stop_pressed;       // STOP button flag (RB0)
extern volatile int1 run_pressed;        // RUN button flag (RB1)
extern volatile int1 cp1_long_press;     // Long-press flag for STOP button

// ============================================================================
// SECTION 6: MOTOR LOOKUP TABLE (ROM)
// Indexed by 3-bit sensor pattern (0b000 to 0b111)
// ============================================================================
extern const MotorCmd_t motor_table[8];

// ============================================================================
// SECTION 7: FUNCTION PROTOTYPES - SYSTEM INITIALIZATION
// ============================================================================
void system_init(void);
void gpio_init(void);
void timers_init(void);
void pwm_init(void);
void interrupts_init(void);

// ============================================================================
// SECTION 8: FUNCTION PROTOTYPES - SENSORS
// ============================================================================

// Initialize sensor GPIO and chatter buffer
void sensors_init(void);

// Read raw 3-bit pattern (Left|Mid|Right), inverted logic (line=1, white=0)
int8 sensors_read_raw(void);

// Chatter filter: must be called from ISR every ~2ms
// Requires CHATTER_N consecutive identical readings before accepting change
void sensors_chatter_filter(int8 new_reading);

// Get current filtered pattern
int8 sensors_read_filtered(void);

// Get/set last known valid line pattern (used when line is lost)
int8 sensors_get_last_valid(void);
void sensors_set_last_valid(int8 pattern);

// Pattern classification helpers
int1 sensors_is_valid_line(int8 pattern);    // Not 000, 101, or 111
int1 sensors_is_intersection(int8 pattern);  // 111: all 3 sensors on line
int1 sensors_is_tjunction(int8 pattern);     // 101: L+R on line, mid off
int1 sensors_is_lost(int8 pattern);          // 000: all white

// LED control (LED1 = status, LED2 = action)
void sensors_led_status(int1 state);
void sensors_led_status_toggle(void);
void sensors_led_action(int1 state);
void sensors_led_action_toggle(void);
void sensors_led_blink_action(void);         // Deprecated: non-blocking blink

// Calibration (stub - digital threshold sensors, no calibration needed)
void sensors_calibrate_start(void);
void sensors_calibrate_update(void);
int1 sensors_is_calibrated(void);

// ============================================================================
// SECTION 9: FUNCTION PROTOTYPES - MOTOR
// ============================================================================

// Initialize software PWM via Timer2
void motor_init(void);

// Core motor control
void motor_set(int8 left_pwm, int8 right_pwm, int1 direction);
MotorCmd_t motor_get_command(int8 pattern); // Lookup table lookup

// High-level movement helpers
void motor_stop(void);
void motor_forward(int8 speed);
void motor_reverse(int8 speed);
void motor_pivot_right(int8 speed);          // Spin in place (right)
void motor_pivot_left(int8 speed);           // Spin in place (left)

// Motor driver enable/disable (STBY pin)
void motor_enable(void);
void motor_disable(void);

// ============================================================================
// SECTION 10: FUNCTION PROTOTYPES - ULTRASONIC
// ============================================================================

// Initialize trigger pin
void ultrasonic_init(void);

// Read distance (returns ULTRA_ERROR=255 on timeout)
int8  ultrasonic_read_cm(void);
int16 ultrasonic_read_mm(void);             // For precise reverse (0xFFFF on error)

// Check if object is within threshold_cm
int1  ultrasonic_check_close(int8 threshold_cm);

// Low-level: generate 10us trigger pulse
void  ultrasonic_trigger(void);

// Low-level: wait for echo and return pulse width in microseconds
int16 ultrasonic_measure_pulse(void);

// ============================================================================
// SECTION 11: FUNCTION PROTOTYPES - FSM
// ============================================================================

// Main state machine transition (calls exit/entry/update hooks)
void fsm_transition(SystemState_t next_state);

// --- State entry functions (called once on entering state) ---
void state_idle_entry(void);
void state_follow_line_entry(void);
void state_station_entry(void);
void state_station_stop_entry(void);
void state_station_back_entry(void);
void state_navigation_entry(void);
void state_end_entry(void);
void state_end_reverse_entry(void);
void state_error_entry(void);
void state_checkpoint_entry(void);

// --- State update functions (called repeatedly in main loop) ---
void state_idle_update(void);
void state_follow_line_update(void);
void state_station_update(void);
void state_station_stop_update(void);
void state_station_back_update(void);
void state_navigation_update(void);
void state_end_update(void);
void state_end_reverse_update(void);
void state_error_update(void);
void state_checkpoint_update(void);

// --- State exit functions (called once on leaving state) ---
void state_idle_exit(void);
void state_follow_line_exit(void);
void state_station_exit(void);
void state_station_stop_exit(void);
void state_station_back_exit(void);
void state_navigation_exit(void);
void state_end_exit(void);
void state_end_reverse_exit(void);
void state_error_exit(void);
void state_checkpoint_exit(void);

// FSM utility functions
void fsm_handle_error(ErrorType_t error);
void fsm_save_checkpoint(Checkpoint_t cp);
void fsm_restore_checkpoint(void);

// ============================================================================
// SECTION 12: FUNCTION PROTOTYPES - PERIPHERALS & MAIN UTILITIES
// ============================================================================

// Servo/relay control for ball grab and release
void peripherals_grab_ball(void);
void peripherals_release_ball(void);

// Checkpoint button handler (called in main loop)
void handle_checkpoint_buttons(void);

// Utility timing helpers
void delay_ms_nonblocking(int16 ms);
int1 check_timeout(int32 start_time, int32 timeout_ms);
int32 get_ms_tick(void);

#endif // MAIN_H