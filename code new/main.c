////////////////////////////////////////////////////////////////////////////////
// FILE: main.c
// DESC: Line follower robot - full implementation (merged from sensors.c,
//       motor.c, ultrasonic.c, fsm.c, main.c)
// DEVICE: PIC18F2685 @ 20MHz
//
// MODULE SECTIONS (use Ctrl+F on section name to jump):
//   [GLOBALS]      - Global variable definitions
//   [SENSORS]      - IR sensor reading & chatter filter
//   [MOTOR]        - Motor driver & PWM control
//   [ULTRASONIC]   - HC-SR04 distance measurement
//   [FSM-STATES]   - State entry / update / exit handlers
//   [FSM-CORE]     - fsm_transition, error handler, checkpoint
//   [ISR]          - Interrupt service routines (Timer0, Timer2, PORTB)
//   [INIT]         - System, GPIO, timers, PWM, interrupt init
//   [MAIN]         - main loop and checkpoint button handler
////////////////////////////////////////////////////////////////////////////////

#include "main.h"

////////////////////////////////////////////////////////////////////////////////
// [GLOBALS] - Global variable definitions
////////////////////////////////////////////////////////////////////////////////

// --- FSM state ---
volatile SystemState_t current_state    = STATE_IDLE;
volatile SystemState_t previous_state   = STATE_IDLE;
ErrorType_t            current_error    = ERR_NONE;
NavDirection_t         nav_direction    = NAV_STRAIGHT;
Checkpoint_t           current_checkpoint = CP_START;
volatile int1          run_flag         = FALSE;
int8                   retry_count      = 0;

// --- Timing (modified in ISR) ---
volatile int32 ms_tick          = 0;
volatile int16 led_tick         = 0;
volatile int1  led2_blink_enable = FALSE;
volatile int8  led2_blink_count  = 0;
volatile int1  programming_mode  = FALSE;   // unused, kept for compatibility
volatile int16 led1_blink_tick   = 0;       // unused, kept for compatibility

// --- Button flags (modified in ISR) ---
volatile int1 system_enabled = FALSE;
volatile int1 stop_pressed   = FALSE;
volatile int1 run_pressed    = FALSE;
volatile int1 cp1_long_press = FALSE;

// --- Motor lookup table (ROM) ---
// Indexed by 3-bit sensor pattern (PATTERN_000 .. PATTERN_111)
// Values: { left_pwm, right_pwm, direction }
// Line = 1 (inverted sensor logic).  Center = PATTERN_010.
const MotorCmd_t motor_table[8] = {
    { STOP_PWM,   STOP_PWM,  DIR_FORWARD },  // 000: Lost line  -> stop
    { BASE_PWM,   SLOW_PWM,  DIR_FORWARD },  // 001: Strong right -> turn left
    { BASE_PWM,   BASE_PWM,  DIR_FORWARD },  // 010: Centered   -> straight
    { BASE_PWM,   SLOW_PWM,  DIR_FORWARD },  // 011: Slight right-> bear left
    { SLOW_PWM,   BASE_PWM,  DIR_FORWARD },  // 100: Strong left -> turn right
    { STOP_PWM,   STOP_PWM,  DIR_FORWARD },  // 101: T-junction -> stop (handled by FSM)
    { SLOW_PWM,   BASE_PWM,  DIR_FORWARD },  // 110: Slight left-> bear right
    { STOP_PWM,   STOP_PWM,  DIR_FORWARD },  // 111: Intersection-> stop (handled by FSM)
};

////////////////////////////////////////////////////////////////////////////////
// [SENSORS] - IR sensor reading and chatter filter
// Sensors are ACTIVE LOW: line = LOW = 1 (inverted), white = HIGH = 0
////////////////////////////////////////////////////////////////////////////////

// Private sensor state
static int8 chatter_buffer[CHATTER_N];  // Circular buffer for debounce
static int8 chatter_index    = 0;       // Write position in buffer
static int8 filtered_pattern = 0b010;   // Current stable pattern (centered)
static int8 last_valid_pattern = 0b010; // Last pattern confirmed on line
static int1 calibrated       = FALSE;   // Calibration flag

// Non-blocking LED blink state (used by deprecated sensors_led_blink_action)
static int16 led_blink_start = 0;
static int1  led_blink_state = 0;

// --- sensors_init ---
// Initialize chatter buffer and LEDs.
void sensors_init(void) {
    int8 i;
    for (i = 0; i < CHATTER_N; i++) {
        chatter_buffer[i] = 0b010;  // Assume centered at startup
    }
    output_low(LED_STATUS);
    output_low(LED_ACTION);
    calibrated = TRUE;  // Digital-threshold sensors need no calibration
}

// --- sensors_read_raw ---
// Returns 3-bit pattern: bit2=Left, bit1=Mid, bit0=Right.
// Inverted: active-low sensor → 1 means line detected.
int8 sensors_read_raw(void) {
    int8 pattern = 0;
    if (!input(SENSOR_LEFT))  pattern |= 0b100;
    if (!input(SENSOR_MID))   pattern |= 0b010;
    if (!input(SENSOR_RIGHT)) pattern |= 0b001;
    return pattern;
}

// --- sensors_chatter_filter ---
// Call from ISR every ~2ms.
// Accepts a new reading into the circular buffer; only updates
// filtered_pattern when all CHATTER_N samples are identical.
void sensors_chatter_filter(int8 new_reading) {
    int8 i;
    int1 all_same;

    chatter_buffer[chatter_index] = new_reading;
    chatter_index = (chatter_index + 1) % CHATTER_N;

    // Check consensus
    all_same = TRUE;
    for (i = 1; i < CHATTER_N; i++) {
        if (chatter_buffer[i] != chatter_buffer[0]) {
            all_same = FALSE;
            break;
        }
    }

    if (all_same) {
        filtered_pattern = chatter_buffer[0];
        // Only persist valid-line patterns as last_valid
        if (sensors_is_valid_line(filtered_pattern)) {
            last_valid_pattern = filtered_pattern;
        }
    }
}

// --- sensors_read_filtered ---
int8 sensors_read_filtered(void) {
    return filtered_pattern;
}

// --- sensors_get_last_valid / sensors_set_last_valid ---
int8 sensors_get_last_valid(void) {
    return last_valid_pattern;
}
void sensors_set_last_valid(int8 pattern) {
    last_valid_pattern = pattern;
}

// --- Pattern classification ---
// Valid line: any pattern except lost(000), T-junction(101), intersection(111)
int1 sensors_is_valid_line(int8 pattern) {
    return (pattern != 0b000 && pattern != 0b101 && pattern != 0b111);
}
// All 3 sensors on line
int1 sensors_is_intersection(int8 pattern) {
    return (pattern == 0b111);
}
// Left+Right on line, middle off
int1 sensors_is_tjunction(int8 pattern) {
    return (pattern == 0b101);
}
// All 3 sensors off line
int1 sensors_is_lost(int8 pattern) {
    return (pattern == 0b000);
}

// --- LED helpers ---
void sensors_led_status(int1 state) {
    if (state) output_high(LED_STATUS);
    else        output_low(LED_STATUS);
}
void sensors_led_status_toggle(void) {
    static int1 led_state = FALSE;
    led_state = !led_state;
    sensors_led_status(led_state);
}
void sensors_led_action(int1 state) {
    if (state) output_high(LED_ACTION);
    else        output_low(LED_ACTION);
}
void sensors_led_action_toggle(void) {
    static int1 led_state = FALSE;
    led_state = !led_state;
    sensors_led_action(led_state);
}
// Deprecated: use ISR-driven toggling instead
void sensors_led_blink_action(void) {
    if ((ms_tick - led_blink_start) >= 250) {
        led_blink_state = !led_blink_state;
        sensors_led_action(led_blink_state);
        led_blink_start = ms_tick;
    }
}

// --- Calibration stubs (digital sensors need no calibration) ---
void sensors_calibrate_start(void)  { calibrated = TRUE; }
void sensors_calibrate_update(void) { }
int1 sensors_is_calibrated(void)    { return calibrated; }

////////////////////////////////////////////////////////////////////////////////
// [MOTOR] - Motor driver control via TB6612FNG and software PWM
//
// Software PWM is generated in timer2_isr (see [ISR] section) for BOTH motors.
// duty_L, duty_R and timer2_count are shared with the ISR (static locals hoisted
// to file scope to allow ISR access).
// Direction control now uses AIN1/AIN2, BIN1/BIN2 for proper H-bridge operation.
////////////////////////////////////////////////////////////////////////////////

// Private motor PWM state (read by timer2_isr)
static int8 duty_L      = 0;    // Left motor duty cycle (software PWM)
static int8 duty_R      = 0;    // Right motor duty cycle (software PWM)
static int8 timer2_count = 0;   // Software PWM counter

// --- motor_init ---
// Set up Timer2 for software PWM at ~2ms tick (500Hz base) for both motors.
void motor_init(void) {
    setup_timer_2(T2_DIV_BY_4, 49, 1);  // Timer2: prescale=4, period=49 -> 2ms @ 20MHz
    duty_L = 0;
    duty_R = 0;
    timer2_count = 0;
    output_low(MOTOR_STBY);             // Disable motor driver at startup
    enable_interrupts(INT_TIMER2);
}

// --- motor_set ---
// Set both motor speeds and individual direction control using PWM.
// left_pwm, right_pwm: 0-255 duty cycle (software PWM)
// direction: DIR_FORWARD(1) -> AIN1/BIN1 HIGH, AIN2/BIN2 LOW
//            DIR_REVERSE(0) -> AIN1/BIN1 LOW, AIN2/BIN2 HIGH
void motor_set(int8 left_pwm, int8 right_pwm, int1 direction) {
    // Set direction: forward (AIN1/BIN1) or reverse (AIN2/BIN2)
    if (direction) {
        // Forward: AIN1=H, AIN2=L; BIN1=H, BIN2=L
        output_high(MOTOR_AIN1);
        output_low(MOTOR_AIN2);
        output_high(MOTOR_BIN1);
        output_low(MOTOR_BIN2);
    } else {
        // Reverse: AIN1=L, AIN2=H; BIN1=L, BIN2=H
        output_low(MOTOR_AIN1);
        output_high(MOTOR_AIN2);
        output_low(MOTOR_BIN1);
        output_high(MOTOR_BIN2);
    }

    // Set PWM duty cycles (software PWM via timer2_isr)
    duty_L = left_pwm;
    duty_R = right_pwm;
}

// --- motor_get_command ---
// Look up motor command from ROM table for given 3-bit sensor pattern.
MotorCmd_t motor_get_command(int8 pattern) {
    return motor_table[pattern & 0x07];
}

// --- High-level movement helpers ---
void motor_stop(void) {
    motor_set(STOP_PWM, STOP_PWM, DIR_FORWARD);
    output_low(MOTOR_STBY);
}

void motor_forward(int8 speed) {
    output_high(MOTOR_STBY);
    motor_set(speed, speed, DIR_FORWARD);
}

void motor_reverse(int8 speed) {
    output_high(MOTOR_STBY);
    motor_set(speed, speed, DIR_REVERSE);
}

// Pivot right: left motor forward, right motor reverse
void motor_pivot_right(int8 speed) {
    output_high(MOTOR_STBY);
    // Left: forward
    output_high(MOTOR_AIN1);
    output_low(MOTOR_AIN2);
    // Right: reverse
    output_low(MOTOR_BIN1);
    output_high(MOTOR_BIN2);
    duty_L = speed;
    duty_R = speed;
}

// Pivot left: right motor forward, left motor reverse
void motor_pivot_left(int8 speed) {
    output_high(MOTOR_STBY);
    // Left: reverse
    output_low(MOTOR_AIN1);
    output_high(MOTOR_AIN2);
    // Right: forward
    output_high(MOTOR_BIN1);
    output_low(MOTOR_BIN2);
    duty_L = speed;
    duty_R = speed;
}

void motor_enable(void)  { output_high(MOTOR_STBY); }
void motor_disable(void) { output_low(MOTOR_STBY);  }

////////////////////////////////////////////////////////////////////////////////
// [ULTRASONIC] - HC-SR04 distance measurement
//
// Timer1 runs at 20MHz / 1 = 20MHz.  1 tick = 0.05us.
// Easier math: pulse measured with delay_us loop (1us per iteration).
// Distance (cm)  = pulse_us / 58
// Distance (mm)  = (pulse_us * 10) / 58
////////////////////////////////////////////////////////////////////////////////

// --- ultrasonic_init ---
void ultrasonic_init(void) {
    output_low(ULTRA_TRIG);
}

// --- ultrasonic_trigger ---
// Generate 10us HIGH pulse on TRIG pin.
void ultrasonic_trigger(void) {
    output_low(ULTRA_TRIG);
    delay_us(2);
    output_high(ULTRA_TRIG);
    delay_us(10);
    output_low(ULTRA_TRIG);
}

// --- ultrasonic_measure_pulse ---
// Waits for ECHO to go HIGH, then measures how long it stays HIGH.
// Returns pulse width in microseconds (1 count ≈ 1us via delay loop).
// Returns 0 on timeout (ULTRA_TIMEOUT exceeded).
int16 ultrasonic_measure_pulse(void) {
    int32 timeout_counter;
    int16 ticks;
    int16 pulse_width;

    // Wait for echo to go HIGH
    timeout_counter = 0;
    while (!input(ULTRA_ECHO)) {
        delay_us(1);
        if (++timeout_counter > ULTRA_TIMEOUT) return 0;
    }

    // Measure HIGH duration using Timer1 ticks
    // Timer1 prescale=1 @ 20MHz: 1 tick = 0.2us -> divide by 5 for us
    set_timer1(0);
    timeout_counter = 0;
    while (input(ULTRA_ECHO)) {
        delay_us(1);
        if (++timeout_counter > ULTRA_TIMEOUT) return 0;
    }

    ticks = get_timer1();
    pulse_width = ticks / 5;    // Convert timer ticks to microseconds
    return pulse_width;
}

// --- ultrasonic_read_cm ---
// Returns distance in cm, or ULTRA_ERROR (255) on timeout.
int8 ultrasonic_read_cm(void) {
    int16 pulse_us;
    int16 distance_cm;

    ultrasonic_trigger();
    delay_us(50);

    pulse_us = ultrasonic_measure_pulse();
    if (pulse_us == 0) return ULTRA_ERROR;

    distance_cm = pulse_us / 58;
    if (distance_cm > 254) return 254;
    return (int8)distance_cm;
}

// --- ultrasonic_read_mm ---
// Returns distance in mm.  Returns 0xFFFF on timeout.
int16 ultrasonic_read_mm(void) {
    int16 pulse_us;
    int32 distance_mm;

    ultrasonic_trigger();
    delay_us(50);

    pulse_us = ultrasonic_measure_pulse();
    if (pulse_us == 0) return 0xFFFF;

    // Fixed-point: mm = (pulse_us * 10) / 58
    distance_mm = ((int32)pulse_us * 10) / 58;
    return (int16)distance_mm;
}

// --- ultrasonic_check_close ---
// Returns TRUE if an object is within threshold_cm.
int1 ultrasonic_check_close(int8 threshold_cm) {
    int8 distance = ultrasonic_read_cm();
    if (distance == ULTRA_ERROR) return FALSE;
    return (distance <= threshold_cm);
}

////////////////////////////////////////////////////////////////////////////////
// [FSM-STATES] - State entry, update, exit handlers
//
// Convention:
//   _entry  - called once when entering a state (setup, start timers)
//   _update - called every main-loop iteration while in that state
//   _exit   - called once when leaving a state (cleanup)
//
// State-local timing uses state_entry_time (set in fsm_transition).
////////////////////////////////////////////////////////////////////////////////

// Shared state timing (set by fsm_transition on every state change)
static int32 state_entry_time   = 0;
static int8  line_lost_counter  = 0;
static int32 nav_start_time     = 0;
static int16 reverse_distance_mm = 0;
static int16 last_mm            = 0;
static int8  intersection_count = 0;
static int8  right_turn_count   = 0;
static int32 stop_press_time    = 0;

// ---- STATE_IDLE ----
// Robot waits for system_enabled AND run_flag to be set.
void state_idle_entry(void) {
    motor_stop();
    sensors_led_status(FALSE);
}
void state_idle_update(void) {
    if (system_enabled && run_flag) {
        run_flag = FALSE;
        fsm_transition(STATE_FOLLOW_LINE);
    }
}
void state_idle_exit(void) { }

// ---- STATE_FOLLOW_LINE ----
// Core line-following loop using sensor lookup table.
// Detects: T-junction (-> NAVIGATION), station approach (-> STATION),
//          END point (-> END), line-lost error (-> ERROR).
void state_follow_line_entry(void) {
    sensors_led_status(TRUE);
    line_lost_counter = 0;
}
void state_follow_line_update(void) {
    int8 pattern;
    int8 distance;
    MotorCmd_t cmd;
    int8 speed_factor;

    pattern  = sensors_read_filtered();
    distance = ultrasonic_read_cm();

    // --- Junction detection ---
    if (sensors_is_intersection(pattern)) {
        intersection_count++;
        motor_forward(SLOW_PWM);
        delay_ms(CROSS_DELAY_MS);       // Cross the intersection fully
        fsm_transition(STATE_NAVIGATION);
        return;
    }
    if (sensors_is_tjunction(pattern)) {
        // Check distance to decide: station or navigation
        if (distance <= STOP_CM_STATION) {
            fsm_transition(STATE_STATION);
        } else {
            fsm_transition(STATE_NAVIGATION);
        }
        return;
    }

    // --- END detection via ultrasonic ---
    if (distance != ULTRA_ERROR && distance <= STOP_CM_END) {
        // Confirm with sensor pattern (should see T or 111 at end marker)
        if (sensors_is_tjunction(pattern) || sensors_is_intersection(pattern)) {
            fsm_transition(STATE_END);
            return;
        }
    }

    // --- Line lost ---
    if (sensors_is_lost(pattern)) {
        line_lost_counter++;
        if (line_lost_counter > (LINE_LOST_MS / 2)) {   // ~2ms per ISR tick
            fsm_handle_error(ERR_LOST_LINE);
            return;
        }
        // Maintain last known direction while searching
        pattern = sensors_get_last_valid();
    } else {
        line_lost_counter = 0;
    }

    // --- Normal line following via lookup table ---
    cmd = motor_get_command(pattern);
    motor_set(cmd.left_pwm, cmd.right_pwm, cmd.direction);
}
void state_follow_line_exit(void) {
    sensors_led_status(FALSE);
}

// ---- STATE_STATION ----
// Slow approach to ball pickup station, stop when very close.
void state_station_entry(void) {
    motor_forward(SLOW_PWM);
    sensors_led_action(TRUE);
}
void state_station_update(void) {
    int8 distance = ultrasonic_read_cm();

    if (distance != ULTRA_ERROR && distance <= STOP_CM_STATION) {
        motor_stop();
        fsm_transition(STATE_STATION_STOP);
    }
    // If line lost while approaching: error recovery
    if (sensors_is_lost(sensors_read_filtered())) {
        fsm_handle_error(ERR_LOST_LINE);
    }
}
void state_station_exit(void) { }

// ---- STATE_STATION_STOP ----
// Robot is stopped at pickup station; wait for mechanical gripper to grab ball.
void state_station_stop_entry(void) {
    sensors_led_action(TRUE);
    // Mechanical gripper starts automatically when robot stops
}
void state_station_stop_update(void) {
    // Wait 2 seconds for mechanical gripper to grab ball
    if (check_timeout(state_entry_time, 2000)) {
        fsm_transition(STATE_STATION_BACK);
    }
}
void state_station_stop_exit(void) {
    sensors_led_action(FALSE);
}

// ---- STATE_STATION_BACK ----
// Reverse away from station using distance measurement.
void state_station_back_entry(void) {
    motor_reverse(SLOW_PWM);
    reverse_distance_mm = 0;
    last_mm = ultrasonic_read_mm();
}
void state_station_back_update(void) {
    int16 current_mm;
    int16 delta;

    current_mm = ultrasonic_read_mm();
    if (current_mm != 0xFFFF && last_mm != 0xFFFF) {
        delta = current_mm - last_mm;   // Positive: moving away
        if (delta > 0) reverse_distance_mm += delta;
    }
    last_mm = current_mm;

    if (reverse_distance_mm >= REVERSE_MM) {
        motor_stop();
        fsm_save_checkpoint(CP_AFTER_NAVIGATION);
        fsm_transition(STATE_NAVIGATION);
    }
}
void state_station_back_exit(void) { }

// ---- STATE_NAVIGATION ----
// Execute a pre-determined turn at a junction (T or intersection).
// nav_direction is set by FSM logic before transitioning here.
void state_navigation_entry(void) {
    nav_start_time = get_ms_tick();
    right_turn_count = 0;

    switch (nav_direction) {
        case NAV_RIGHT:   motor_pivot_right(BASE_PWM); break;
        case NAV_LEFT:    motor_pivot_left(BASE_PWM);  break;
        case NAV_UTURN:   motor_pivot_right(BASE_PWM); break; // U-turn = long right pivot
        default:          motor_forward(BASE_PWM);     break; // NAV_STRAIGHT
    }
}
void state_navigation_update(void) {
    int8 pattern;
    int32 turn_duration;

    // Enforce minimum turn time to prevent immediate re-detection
    if (!check_timeout(nav_start_time, NAV_MIN_MS)) return;

    pattern = sensors_read_filtered();

    // For U-turn: wait until centered on line again
    if (nav_direction == NAV_UTURN) {
        if (pattern == PATTERN_010) {
            motor_stop();
            fsm_transition(STATE_FOLLOW_LINE);
        }
        return;
    }

    // For other turns: stop when line centered
    if (sensors_is_valid_line(pattern)) {
        motor_stop();
        delay_ms(50);               // Settle before continuing
        fsm_transition(STATE_FOLLOW_LINE);
    }
}
void state_navigation_exit(void) {
    nav_direction = NAV_STRAIGHT;   // Reset direction for next junction
}

// ---- STATE_END ----
// Robot detected END marker (distance <= 5cm); stop and prepare for release.
void state_end_entry(void) {
    motor_stop();
    sensors_led_action(TRUE);
    sensors_led_status(TRUE);
    fsm_save_checkpoint(CP_BEFORE_END);
}
void state_end_update(void) {
    // Transition to push phase immediately
    fsm_transition(STATE_END_PUSH);
}
void state_end_exit(void) { }

// ---- STATE_END_PUSH ----
// Continue pushing forward 300ms to fully engage release mechanism.
void state_end_push_entry(void) {
    motor_forward(SLOW_PWM);
}
void state_end_push_update(void) {
    // Push for 300ms then transition to wait state
    if (check_timeout(state_entry_time, 300)) {
        motor_stop();
        fsm_transition(STATE_END_WAIT);
    }
}
void state_end_push_exit(void) { }

// ---- STATE_END_WAIT ----
// Wait 2 seconds for mechanical release mechanism to drop ball into container.
void state_end_wait_entry(void) {
    // Motors already stopped by STATE_END_PUSH exit
}
void state_end_wait_update(void) {
    // Wait 2 seconds for ball to drop
    if (check_timeout(state_entry_time, 2000)) {
        fsm_transition(STATE_END_REVERSE);
    }
}
void state_end_wait_exit(void) { }

// ---- STATE_END_REVERSE ----
// Reverse away from end zone; stop only when all 3 sensors are white (000).
void state_end_reverse_entry(void) {
    motor_reverse(SLOW_PWM);
}
void state_end_reverse_update(void) {
    int8 pattern = sensors_read_filtered();

    // Stop reversing only when all 3 sensors detect white (pattern 000)
    if (pattern == PATTERN_000) {
        motor_stop();
        sensors_led_action(FALSE);
        sensors_led_status(FALSE);
        // Mission complete: return to idle
        fsm_transition(STATE_IDLE);
    }
}
void state_end_reverse_exit(void) {
    motor_stop();
}

// ---- STATE_ERROR ----
// Attempt to recover from errors:
//   ERR_LOST_LINE  -> pivot toward last valid direction
//   ERR_BALL_GRAB  -> retry grab sequence
//   Others         -> stop and wait
void state_error_entry(void) {
    motor_stop();
}
void state_error_update(void) {
    int8 pattern;

    switch (current_error) {
        case ERR_LOST_LINE:
            // Pivot in direction of last valid pattern to search for line
            if (last_valid_pattern & 0b100) {   // Was going left-biased
                motor_pivot_left(SLOW_PWM);
            } else {
                motor_pivot_right(SLOW_PWM);
            }
            pattern = sensors_read_filtered();
            if (sensors_is_valid_line(pattern)) {
                motor_stop();
                current_error = ERR_NONE;
                fsm_transition(STATE_FOLLOW_LINE);
            }
            // Timeout: give up after LINE_LOST_MS * 3
            if (check_timeout(state_entry_time, LINE_LOST_MS * 3)) {
                motor_stop();
                fsm_transition(STATE_CHECKPOINT);   // Restore last checkpoint
            }
            break;

        case ERR_ULTRASONIC:
        case ERR_NOISE:
        case ERR_BALL_GRAB:  // Mechanical grab: no special handling needed
        default:
            // Non-fatal: wait briefly then resume
            if (check_timeout(state_entry_time, 200)) {
                current_error = ERR_NONE;
                fsm_transition(STATE_FOLLOW_LINE);
            }
            break;
    }
}
void state_error_exit(void) {
    retry_count = 0;
}

// ---- STATE_CHECKPOINT ----
// Restore robot to last saved checkpoint after error.
void state_checkpoint_entry(void) {
    motor_stop();
}
void state_checkpoint_update(void) {
    // Read saved state from checkpoint
    switch (current_checkpoint) {
        case CP_START:
            // Return to beginning: just restart line following
            nav_direction = NAV_STRAIGHT;
            intersection_count = 0;
            fsm_transition(STATE_FOLLOW_LINE);
            break;

        case CP_AFTER_NAVIGATION:
            // Resume from post-navigation
            fsm_transition(STATE_FOLLOW_LINE);
            break;

        case CP_BEFORE_END:
            // Approaching end zone; resume carefully
            fsm_transition(STATE_FOLLOW_LINE);
            break;

        default:
            fsm_transition(STATE_IDLE);
            break;
    }
}
void state_checkpoint_exit(void) { }

////////////////////////////////////////////////////////////////////////////////
// [FSM-CORE] - State machine transition engine, error handler, checkpoint save
////////////////////////////////////////////////////////////////////////////////

// --- fsm_transition ---
// Performs: exit(current) -> update state -> entry(next)
// Also records state_entry_time for timeout checks.
void fsm_transition(SystemState_t next_state) {
    // ---- Call exit handler for current state ----
    switch (current_state) {
        case STATE_IDLE:           state_idle_exit();           break;
        case STATE_FOLLOW_LINE:    state_follow_line_exit();    break;
        case STATE_STATION:        state_station_exit();        break;
        case STATE_STATION_STOP:   state_station_stop_exit();   break;
        case STATE_STATION_BACK:   state_station_back_exit();   break;
        case STATE_NAVIGATION:     state_navigation_exit();     break;
        case STATE_END:            state_end_exit();            break;
        case STATE_END_PUSH:       state_end_push_exit();       break;
        case STATE_END_WAIT:       state_end_wait_exit();       break;
        case STATE_END_REVERSE:    state_end_reverse_exit();    break;
        case STATE_ERROR:          state_error_exit();          break;
        case STATE_CHECKPOINT:     state_checkpoint_exit();     break;
        default: break;
    }

    // ---- Update state variables ----
    previous_state = current_state;
    current_state  = next_state;
    state_entry_time = get_ms_tick();   // Record entry time for timeouts

    // ---- Call entry handler for new state ----
    switch (current_state) {
        case STATE_IDLE:           state_idle_entry();           break;
        case STATE_FOLLOW_LINE:    state_follow_line_entry();    break;
        case STATE_STATION:        state_station_entry();        break;
        case STATE_STATION_STOP:   state_station_stop_entry();   break;
        case STATE_STATION_BACK:   state_station_back_entry();   break;
        case STATE_NAVIGATION:     state_navigation_entry();     break;
        case STATE_END:            state_end_entry();            break;
        case STATE_END_PUSH:       state_end_push_entry();       break;
        case STATE_END_WAIT:       state_end_wait_entry();       break;
        case STATE_END_REVERSE:    state_end_reverse_entry();    break;
        case STATE_ERROR:          state_error_entry();          break;
        case STATE_CHECKPOINT:     state_checkpoint_entry();     break;
        default: break;
    }
}

// --- fsm_handle_error ---
// Set error type and transition to ERROR state.
void fsm_handle_error(ErrorType_t error) {
    current_error = error;
    fsm_transition(STATE_ERROR);
}

// --- fsm_save_checkpoint ---
// Save current checkpoint marker to EEPROM.
// Layout (1 byte each): cp, nav_direction, intersection_count
void fsm_save_checkpoint(Checkpoint_t cp) {
    current_checkpoint = cp;
    write_eeprom(0, (int8)cp);
    write_eeprom(1, (int8)nav_direction);
    write_eeprom(2, intersection_count);
}

// --- fsm_restore_checkpoint ---
// Load checkpoint data back from EEPROM.
void fsm_restore_checkpoint(void) {
    current_checkpoint = (Checkpoint_t)read_eeprom(0);
    nav_direction      = (NavDirection_t)read_eeprom(1);
    intersection_count = read_eeprom(2);
}

////////////////////////////////////////////////////////////////////////////////
// [ISR] - Interrupt service routines
//
// Timer0 ISR  (~1ms):  ms_tick increment, sensor chatter filter, LED blink
// Timer2 ISR  (~2ms):  Software PWM for right motor (duty_R)
// PORTB ISR   (change): Button debounce for SYSTEM, STOP, RUN buttons
////////////////////////////////////////////////////////////////////////////////

// --- Timer0 ISR (~1ms tick) ---
// Increments ms_tick, runs sensor chatter filter, manages LED blink sequences.
#INT_TIMER0
void timer0_isr(void) {
    int8 raw_pattern;
    int8 current_pattern;

    // Reload Timer0 for 1ms period @ 20MHz, prescale=8
    // Reload = 65536 - (20000000 / 8 / 1000) = 65536 - 2500 = 63036
    set_timer0(63036);

    ms_tick++;

    // --- Sensor chatter filter (every 2ms via led_tick parity) ---
    raw_pattern = sensors_read_raw();
    sensors_chatter_filter(raw_pattern);

    // --- LED1 status blink (500ms period when system enabled) ---
    led_tick++;
    if (system_enabled && current_state != STATE_IDLE) {
        if (led_tick >= 500) {
            led_tick = 0;
            sensors_led_status_toggle();
        }
    }

    // --- LED2 action blink sequence ---
    if (led2_blink_enable && led2_blink_count > 0) {
        if (led_tick % 200 == 0) {
            sensors_led_action_toggle();
            led2_blink_count--;
            if (led2_blink_count == 0) {
                led2_blink_enable = FALSE;
                sensors_led_action(FALSE);
            }
        }
    }
}

// --- Timer2 ISR (~2ms): Software PWM for both motors ---
// Generates software PWM on MOTOR_PWMA (left) and MOTOR_PWMB (right)
// by comparing timer2_count with duty_L and duty_R.
#INT_TIMER2
void timer2_isr(void) {
    output_bit(MOTOR_PWMA, timer2_count < duty_L);
    output_bit(MOTOR_PWMB, timer2_count < duty_R);
    timer2_count++;
    if (timer2_count >= 100) {
        timer2_count = 0;
    }
}

// --- PORTB change ISR: button debounce ---
// Handles SYSTEM (RB7), STOP (RB0), RUN (RB1) buttons.
// Long-press on STOP (>1s) sets cp1_long_press for checkpoint-1 restore.
#INT_RB
void portb_change_isr(void) {
    int8 portb;
    int32 press_duration;

    portb = input_b();  // Read and clear mismatch

    // --- SYSTEM button (RB7): toggle system_enabled ---
    if (!input(BUTTON_SYSTEM)) {
        system_enabled = !system_enabled;
        if (!system_enabled) {
            // System turned OFF: stop everything
            motor_stop();
            sensors_led_status(FALSE);
            sensors_led_action(FALSE);
            current_state = STATE_IDLE;
        }
    }

    // --- STOP button (RB0): save checkpoint / long-press restore ---
    if (!input(BUTTON_STOP)) {
        stop_press_time = get_ms_tick();
        stop_pressed = TRUE;
    } else if (stop_pressed) {
        // Button released: check duration
        press_duration = get_ms_tick() - stop_press_time;
        stop_pressed = FALSE;
        if (press_duration >= 1000) {
            cp1_long_press = TRUE;  // Long press: restore checkpoint 1
        } else {
            // Short press: save current checkpoint
            fsm_save_checkpoint(current_checkpoint);
            fsm_transition(STATE_CHECKPOINT);
        }
    }

    // --- RUN button (RB1): resume from checkpoint ---
    if (!input(BUTTON_RUN)) {
        run_pressed = TRUE;
        run_flag    = TRUE;
    } else {
        run_pressed = FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////
// [INIT] - System and peripheral initialization
////////////////////////////////////////////////////////////////////////////////

// --- gpio_init ---
// Configure TRIS registers and pull-ups.
void gpio_init(void) {
    // PORTA: RA0-RA2 inputs (sensors), RA3-RA4 outputs (motor direction), rest outputs
    set_tris_a(0b00000111);

    // PORTB: RB0,RB1,RB4,RB5,RB7 inputs (buttons + ultrasonic echo)
    //        RB2,RB3,RB6 outputs (LEDs, STBY)
    set_tris_b(0b10110011);
    port_b_pullups(0b10000011);  // Pull-ups on RB0, RB1, RB7

    // PORTC: all outputs (motor driver)
    set_tris_c(0b00000000);

    // Disable analog inputs (use all pins as digital)
    setup_adc(ADC_OFF);
    setup_adc_ports(NO_ANALOGS);

    // Initial output states
    output_low(MOTOR_STBY);
    output_low(MOTOR_AIN1);
    output_low(MOTOR_AIN2);
    output_low(MOTOR_BIN1);
    output_low(MOTOR_BIN2);
    output_low(LED_STATUS);
    output_low(LED_ACTION);
    output_low(ULTRA_TRIG);
}

// --- timers_init ---
// Timer0: 1ms tick (ms_tick, sensor filter, LED blink)
// Timer1: free-running for ultrasonic pulse measurement
// Timer2: Software PWM for both motors (initialized in motor_init)
void timers_init(void) {
    // Timer0: internal clock, prescale=8, ~1ms period
    setup_timer_0(RTCC_INTERNAL | RTCC_DIV_8);
    set_timer0(63036);

    // Timer1: internal clock, prescale=1, free-running for ultrasonic
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_1);
}

// --- interrupts_init ---
void interrupts_init(void) {
    enable_interrupts(INT_TIMER0);
    enable_interrupts(INT_TIMER2);
    enable_interrupts(INT_RB);
    enable_interrupts(GLOBAL);
}

// --- system_init ---
// Master initialization: calls all sub-inits in correct order.
void system_init(void) {
    gpio_init();
    timers_init();
    motor_init();     // Sets up Timer2 for software PWM (both motors)
    sensors_init();   // Initializes chatter buffer, LEDs
    ultrasonic_init();
    interrupts_init();

    // FSM initial state
    current_state    = STATE_IDLE;
    previous_state   = STATE_IDLE;
    current_error    = ERR_NONE;
    nav_direction    = NAV_STRAIGHT;
    run_flag         = FALSE;
    system_enabled   = FALSE;
    retry_count      = 0;
    intersection_count = 0;
    right_turn_count = 0;
}

////////////////////////////////////////////////////////////////////////////////
// [MAIN] - Main loop and checkpoint button handler
////////////////////////////////////////////////////////////////////////////////

// --- handle_checkpoint_buttons ---
// Called from main loop.  Handles long-press restore and RUN resume.
void handle_checkpoint_buttons(void) {
    if (cp1_long_press) {
        cp1_long_press = FALSE;
        // Restore to checkpoint 1 (start of run)
        fsm_restore_checkpoint();
        if (current_checkpoint == CP_START || current_checkpoint < CP_START) {
            // Full reset to start
            nav_direction      = NAV_STRAIGHT;
            intersection_count = 0;
            current_checkpoint = CP_START;
        }
        fsm_transition(STATE_CHECKPOINT);
    }
}

// --- get_ms_tick ---
// Thread-safe read of ms_tick (disable interrupts to prevent torn read on int32).
int32 get_ms_tick(void) {
    int32 t;
    disable_interrupts(GLOBAL);
    t = ms_tick;
    enable_interrupts(GLOBAL);
    return t;
}

// --- check_timeout ---
// Returns TRUE if (current_time - start_time) >= timeout_ms.
int1 check_timeout(int32 start_time, int32 timeout_ms) {
    return ((get_ms_tick() - start_time) >= timeout_ms);
}

// --- delay_ms_nonblocking ---
// Spin-wait using ms_tick (interrupts must remain enabled).
void delay_ms_nonblocking(int16 ms) {
    int32 start = get_ms_tick();
    while (!check_timeout(start, (int32)ms));
}

// ============================================================================
// MAIN
// ============================================================================
void main(void) {
    system_init();

    // Run FSM initial entry
    state_idle_entry();

    while (TRUE) {
        // --- System guard: do nothing if disabled ---
        if (!system_enabled) {
            delay_ms(10);
            continue;
        }

        // --- Process checkpoint / RUN button flags from ISR ---
        handle_checkpoint_buttons();

        // --- Run active state's update handler ---
        switch (current_state) {
            case STATE_IDLE:           state_idle_update();           break;
            case STATE_FOLLOW_LINE:    state_follow_line_update();    break;
            case STATE_STATION:        state_station_update();        break;
            case STATE_STATION_STOP:   state_station_stop_update();   break;
            case STATE_STATION_BACK:   state_station_back_update();   break;
            case STATE_NAVIGATION:     state_navigation_update();     break;
            case STATE_END:            state_end_update();            break;
            case STATE_END_PUSH:       state_end_push_update();       break;
            case STATE_END_WAIT:       state_end_wait_update();       break;
            case STATE_END_REVERSE:    state_end_reverse_update();    break;
            case STATE_ERROR:          state_error_update();          break;
            case STATE_CHECKPOINT:     state_checkpoint_update();     break;
            default:
                // Unknown state: safe fallback
                fsm_transition(STATE_IDLE);
                break;
        }
    }
}