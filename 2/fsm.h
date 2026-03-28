// ============================================================================
// FILE: fsm.h
// PROJECT: Line Follower Robot - PIC18F2685  
// DESCRIPTION: Finite State Machine declarations and transitions
// ============================================================================

#ifndef FSM_H
#define FSM_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// STATE MACHINE ENUMERATION
// ============================================================================

typedef enum {
    STATE_IDLE = 0,          // Waiting for RUN button
    STATE_FOLLOW_LINE,       // Normal line following
    STATE_STATION,           // Approaching station (ultrasonic active)
    STATE_STATION_STOP,      // Stopped at station, grabbing ball
    STATE_STATION_BACK,      // Backing up from station
    STATE_NAVIGATION,        // Handling T-junction / intersection turns  
    STATE_END,               // At END point, dropping ball
    STATE_ERROR,             // Error recovery state
    STATE_CHECKPOINT_1,      // Checkpoint 1 recovery
    STATE_CHECKPOINT_2       // Checkpoint 2 recovery
} SystemState_t;

// ============================================================================
// ERROR TYPES
// ============================================================================

typedef enum {
    ERR_NONE = 0,           // No error
    ERR_NOISE,              // Sensor noise detected
    ERR_LOSTLINE,           // Line completely lost
    ERR_TIMEOUT,            // Operation timeout
    ERR_SENSOR_FAIL         // Sensor failure
} ErrorType_t;

// ============================================================================
// STATE FUNCTION PROTOTYPES
// ============================================================================

// State transition controller
void transition(SystemState_t new_state);

// STATE_IDLE
void idle_entry(void);
void idle_update(void);
void idle_exit(void);

// STATE_FOLLOW_LINE
void follow_line_entry(void);
void follow_line_update(void);
void follow_line_exit(void);

// STATE_STATION
void station_entry(void);
void station_update(void);
void station_exit(void);

// STATE_STATION_STOP
void station_stop_entry(void);
void station_stop_update(void);
void station_stop_exit(void);

// STATE_STATION_BACK
void station_back_entry(void);
void station_back_update(void);
void station_back_exit(void);

// STATE_NAVIGATION
void navigation_entry(void);
void navigation_update(void);
void navigation_exit(void);

// STATE_END  
void end_entry(void);
void end_update(void);
void end_exit(void);

// STATE_ERROR
void error_entry(void);
void error_update(void);
void error_exit(void);

// STATE_CHECKPOINT_1
void checkpoint1_entry(void);
void checkpoint1_update(void);
void checkpoint1_exit(void);

// STATE_CHECKPOINT_2
void checkpoint2_entry(void);
void checkpoint2_update(void);
void checkpoint2_exit(void);

#endif // FSM_H