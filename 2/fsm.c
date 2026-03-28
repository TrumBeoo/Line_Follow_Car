// ============================================================================
// FILE: fsm.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: Finite State Machine implementation (10 states)
// ============================================================================

#include "fsm.h"
#include "main.h"

// ============================================================================
// STATIC VARIABLES FOR FSM
// ============================================================================

static uint32_t state_entry_time = 0;      // Timestamp when entering state
static uint32_t lost_line_time = 0;        // Timestamp when line was lost
static uint32_t operation_start_time = 0;  // Generic operation timer
static bool turn_complete = false;         // Turn completion flag
static uint8_t station_pass_count = 0;     // Count stations passed

// ============================================================================
// STATE TRANSITION CONTROLLER
// ============================================================================

/**
 * Transition to new state with proper cleanup
 * Calls exit() of old state, then entry() of new state
 */
void transition(SystemState_t new_state) {
    // Call exit function of current state
    switch(current_state) {
        case STATE_IDLE:          idle_exit(); break;
        case STATE_FOLLOW_LINE:   follow_line_exit(); break;
        case STATE_STATION:       station_exit(); break;
        case STATE_STATION_STOP:  station_stop_exit(); break;
        case STATE_STATION_BACK:  station_back_exit(); break;
        case STATE_NAVIGATION:    navigation_exit(); break;
        case STATE_END:           end_exit(); break;
        case STATE_ERROR:         error_exit(); break;
        case STATE_CHECKPOINT_1:  checkpoint1_exit(); break;
        case STATE_CHECKPOINT_2:  checkpoint2_exit(); break;
    }
    
    // Update state
    current_state = new_state;
    state_entry_time = ms_tick;
    
    // Call entry function of new state
    switch(current_state) {
        case STATE_IDLE:          idle_entry(); break;
        case STATE_FOLLOW_LINE:   follow_line_entry(); break;
        case STATE_STATION:       station_entry(); break;
        case STATE_STATION_STOP:  station_stop_entry(); break;
        case STATE_STATION_BACK:  station_back_entry(); break;
        case STATE_NAVIGATION:    navigation_entry(); break;
        case STATE_END:           end_entry(); break;
        case STATE_ERROR:         error_entry(); break;
        case STATE_CHECKPOINT_1:  checkpoint1_entry(); break;
        case STATE_CHECKPOINT_2:  checkpoint2_entry(); break;
    }
}

// ============================================================================
// STATE: IDLE - Waiting for RUN button
// ============================================================================

void idle_entry(void) {
    motor_stop();
    motor_disable();
    led_status_off();
    led_action_off();
    run_flag = false;
    error_count = 0;
}

void idle_update(void) {
    // Wait for RUN button press
    if(run_flag) {
        run_flag = false;
        
        // Start based on checkpoint
        if(checkpoint_state == 0) {
            transition(STATE_FOLLOW_LINE);
        } else if(checkpoint_state == 1) {
            transition(STATE_CHECKPOINT_1);
        } else if(checkpoint_state == 2) {
            transition(STATE_CHECKPOINT_2);
        }
    }
}

void idle_exit(void) {
    motor_enable();
    led_status_on();
}

// ============================================================================
// STATE: FOLLOW_LINE - Normal line following with lookup table
// ============================================================================

void follow_line_entry(void) {
    sensor_pattern = read_sensor_pattern();
    last_valid_pattern = sensor_pattern;
}

void follow_line_update(void) {
    // Read and filter sensor
    uint8_t raw_pattern = read_sensor_pattern();
    sensor_pattern = chatter_filter(raw_pattern);
    
    // Check for STOP button
    if(stop_flag) {
        stop_flag = false;
        transition(STATE_IDLE);
        return;
    }
    
    // Check for intersection (all sensors black)
    if(is_intersection(sensor_pattern)) {
        transition(STATE_NAVIGATION);
        return;
    }
    
    // Check for T-junction
    if(is_t_junction(sensor_pattern)) {
        transition(STATE_NAVIGATION);
        return;
    }
    
    // Check for line loss
    if(sensor_pattern == PATTERN_LOST) {
        // Start line loss timer if not already started
        if(lost_line_time == 0) {
            lost_line_time = ms_tick;
        }
        
        // Maintain last direction for LOST_TIMEOUT_MS
        if((ms_tick - lost_line_time) < LOST_TIMEOUT_MS) {
            motor_apply_table(last_valid_pattern);
        } else {
            // Timeout - transition to error state
            error_type = ERR_LOSTLINE;
            transition(STATE_ERROR);
            return;
        }
    } else {
        // Valid line detected
        lost_line_time = 0;  // Reset lost line timer
        last_valid_pattern = sensor_pattern;
        
        // Apply motor command from lookup table
        motor_apply_table(sensor_pattern);
    }
    
    // Check ultrasonic for station or END
    uint8_t distance = hcsr04_read_cm();
    if(distance != TIMEOUT_CM) {
        if(distance <= STOP_CM_END && station_pass_count >= 1) {
            // At END point after passing station
            transition(STATE_END);
        } else if(distance <= STOP_CM_STATION) {
            // At station
            transition(STATE_STATION);
        }
    }
}

void follow_line_exit(void) {
    // Nothing to clean up
}

// ============================================================================
// STATE: STATION - Approaching station with ultrasonic active
// ============================================================================

void station_entry(void) {
    // Reduce speed to 50% for careful approach
    motor_forward(BASE_PWM / 2);
}

void station_update(void) {
    // Continuously measure distance
    uint8_t distance = hcsr04_read_cm();
    
    // Stop when very close (≤2cm)
    if(distance <= STOP_CM_STATION && distance != TIMEOUT_CM) {
        transition(STATE_STATION_STOP);
        return;
    }
    
    // Light line following while approaching
    uint8_t raw_pattern = read_sensor_pattern();
    sensor_pattern = chatter_filter(raw_pattern);
    
    if(sensor_pattern != PATTERN_LOST) {
        motor_apply_table(sensor_pattern);
    }
}

void station_exit(void) {
    motor_stop();
}

// ============================================================================
// STATE: STATION_STOP - Stopped at station, grabbing ball
// ============================================================================

void station_stop_entry(void) {
    motor_stop();
    led_action_on();
    
    // Activate ball grab mechanism (servo or solenoid)
    output_high(SERVO_GRAB_PIN);
    
    operation_start_time = ms_tick;
    ball_ok = false;
}

void station_stop_update(void) {
    // Wait for GRAB_MS or ball detection
    if(ball_ok || (ms_tick - operation_start_time) >= GRAB_MS) {
        station_pass_count++;
        transition(STATE_STATION_BACK);
    }
}

void station_stop_exit(void) {
    output_low(SERVO_GRAB_PIN);
    led_action_off();
}

// ============================================================================
// STATE: STATION_BACK - Backing up 250mm from station
// ============================================================================

void station_back_entry(void) {
    operation_start_time = ms_tick;
    motor_reverse(BACK_PWM);
}

void station_back_update(void) {
    // Backup for estimated time (250mm at BACK_PWM ≈ 500ms)
    // Use ultrasonic to measure distance backed up
    if((ms_tick - operation_start_time) >= BACK_DELAY_MS) {
        transition(STATE_NAVIGATION);
    }
}

void station_back_exit(void) {
    motor_stop();
}

// ============================================================================
// STATE: NAVIGATION - Handling turns at T-junction / intersection
// ============================================================================

void navigation_entry(void) {
    // Ignore sensor temporarily, go straight to clear intersection
    motor_forward(BASE_PWM);
    operation_start_time = ms_tick;
    turn_complete = false;
}

void navigation_update(void) {
    // Phase 1: Go straight for CROSS_DELAY_MS to clear intersection
    if(!turn_complete && (ms_tick - operation_start_time) < CROSS_DELAY_MS) {
        motor_forward(BASE_PWM);
        return;
    }
    
    // Phase 2: Pivot turn right (after station) or handle intersection
    if(!turn_complete) {
        turn_complete = true;
        operation_start_time = ms_tick;
        motor_pivot_right(TURN_PWM);
    }
    
    // Phase 3: Keep turning until middle sensor detects line
    if(turn_complete) {
        uint8_t raw_pattern = read_sensor_pattern();
        
        // Check if middle sensor sees line and turn duration met
        if((raw_pattern & 0b010) && (ms_tick - operation_start_time) > TURN_DELAY_MS) {
            // Line re-acquired, return to following
            transition(STATE_FOLLOW_LINE);
        }
        
        // Timeout protection
        if((ms_tick - operation_start_time) > 2000) {
            error_type = ERR_TIMEOUT;
            transition(STATE_ERROR);
        }
    }
}

void navigation_exit(void) {
    motor_stop();
}

// ============================================================================
// STATE: END - At END point, dropping ball
// ============================================================================

void end_entry(void) {
    motor_stop();
    led_status_off();
    
    // Activate ball drop mechanism
    output_high(DROP_PIN);
    led_action_on();
    
    operation_start_time = ms_tick;
}

void end_update(void) {
    // Drop ball for DROP_MS
    if((ms_tick - operation_start_time) >= DROP_MS) {
        output_low(DROP_PIN);
        led_action_off();
        
        // Reset for new cycle
        station_pass_count = 0;
        checkpoint_state = 0;
        
        // Return to IDLE
        transition(STATE_IDLE);
    }
}

void end_exit(void) {
    output_low(DROP_PIN);
}

// ============================================================================
// STATE: ERROR - Error recovery with retry logic
// ============================================================================

void error_entry(void) {
    motor_stop();
    led_status_on();
    led_action_on();
    error_count++;
}

void error_update(void) {
    // Flash LEDs to indicate error
    if((ms_tick % 200) < 100) {
        led_status_on();
        led_action_on();
    } else {
        led_status_off();
        led_action_off();
    }
    
    // Check error type and retry
    if(error_type == ERR_LOSTLINE && error_count < 3) {
        // Try backing up and re-finding line
        motor_reverse(BACK_PWM);
        delay_ms(300);
        motor_stop();
        
        // Try to find line again
        uint8_t pattern = read_sensor_pattern();
        if(is_valid_pattern(pattern)) {
            error_type = ERR_NONE;
            error_count = 0;
            transition(STATE_FOLLOW_LINE);
            return;
        }
    }
    
    // If too many errors, wait for manual intervention
    if(error_count >= 3) {
        motor_stop();
        // Wait for STOP then RUN button
        if(stop_flag) {
            stop_flag = false;
            error_count = 0;
            error_type = ERR_NONE;
            transition(STATE_IDLE);
        }
    }
}

void error_exit(void) {
    led_status_off();
    led_action_off();
}

// ============================================================================
// STATE: CHECKPOINT_1 - Resume from checkpoint 1
// ============================================================================

void checkpoint1_entry(void) {
    led_status_on();
    checkpoint_state = 1;
}

void checkpoint1_update(void) {
    // Position at checkpoint 1, resume normal operation
    transition(STATE_FOLLOW_LINE);
}

void checkpoint1_exit(void) {
    // Nothing
}

// ============================================================================
// STATE: CHECKPOINT_2 - Resume from checkpoint 2
// ============================================================================

void checkpoint2_entry(void) {
    led_status_on();
    checkpoint_state = 2;
    station_pass_count = 1;  // Already passed station
}

void checkpoint2_update(void) {
    // Position at checkpoint 2, resume normal operation
    transition(STATE_FOLLOW_LINE);
}

void checkpoint2_exit(void) {
    // Nothing
}