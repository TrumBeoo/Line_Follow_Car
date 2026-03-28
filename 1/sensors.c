////////////////////////////////////////////////////////////////////////////////
// FILE: sensors.c
// DESC: IR sensor reading with chatter filter implementation
////////////////////////////////////////////////////////////////////////////////

#include "main.h"

// ============================================================================
// PRIVATE VARIABLES
// ============================================================================
static int8 chatter_buffer[CHATTER_N];  // Circular buffer for filtering
static int8 chatter_index = 0;           // Current position in buffer
static int8 filtered_pattern = 0b010;    // Current filtered pattern
static int8 last_valid_pattern = 0b010;  // Last valid line pattern
static int1 calibrated = FALSE;          // Calibration status

// For LED blinking (non-blocking)
static int16 led_blink_start = 0;
static int1 led_blink_state = 0;

// ============================================================================
// SENSOR INITIALIZATION
// ============================================================================
void sensors_init(void) {
    // Initialize chatter buffer
    for (int8 i = 0; i < CHATTER_N; i++) {
        chatter_buffer[i] = 0b010;  // Start with centered pattern
    }
    
    // Initialize LEDs
    output_low(LED_STATUS);
    output_low(LED_ACTION);
    
    calibrated = TRUE;  // Skip calibration for now (can be implemented later)
}

// ============================================================================
// READ RAW SENSOR PATTERN
// Sensors are ACTIVE LOW (line = 0, white = 1)
// We invert to get: line = 1, white = 0
// ============================================================================
int8 sensors_read_raw(void) {
    int8 pattern = 0;
    
    // Read sensors (inverted logic)
    if (!input(SENSOR_LEFT))  pattern |= 0b100;  // Left sensor
    if (!input(SENSOR_MID))   pattern |= 0b010;  // Middle sensor  
    if (!input(SENSOR_RIGHT)) pattern |= 0b001;  // Right sensor
    
    return pattern;
}

// ============================================================================
// CHATTER FILTER (called from ISR every 2ms)
// Requires N consecutive identical readings to accept change
// ============================================================================
void sensors_chatter_filter(int8 new_reading) {
    // Add new reading to buffer
    chatter_buffer[chatter_index] = new_reading;
    chatter_index = (chatter_index + 1) % CHATTER_N;
    
    // Check if all N samples are identical
    int1 all_same = TRUE;
    for (int8 i = 1; i < CHATTER_N; i++) {
        if (chatter_buffer[i] != chatter_buffer[0]) {
            all_same = FALSE;
            break;
        }
    }
    
    // If all samples match, update filtered pattern
    if (all_same) {
        filtered_pattern = chatter_buffer[0];
        
        // Update last valid pattern if this is a valid line
        if (sensors_is_valid_line(filtered_pattern)) {
            last_valid_pattern = filtered_pattern;
        }
    }
}

// ============================================================================
// READ FILTERED SENSOR PATTERN
// ============================================================================
int8 sensors_read_filtered(void) {
    return filtered_pattern;
}

// ============================================================================
// GET LAST VALID PATTERN
// ============================================================================
int8 sensors_get_last_valid(void) {
    return last_valid_pattern;
}

// ============================================================================
// SET LAST VALID PATTERN
// ============================================================================
void sensors_set_last_valid(int8 pattern) {
    last_valid_pattern = pattern;
}

// ============================================================================
// CHECK IF PATTERN IS VALID LINE (not lost, not intersection)
// Valid patterns: 001, 010, 011, 100, 110
// Invalid: 000 (lost), 101 (T-junction), 111 (intersection)
// ============================================================================
int1 sensors_is_valid_line(int8 pattern) {
    return (pattern != 0b000 && pattern != 0b101 && pattern != 0b111);
}

// ============================================================================
// CHECK IF INTERSECTION (all sensors on line)
// ============================================================================
int1 sensors_is_intersection(int8 pattern) {
    return (pattern == 0b111);
}

// ============================================================================
// CHECK IF T-JUNCTION (left and right on line, middle off)
// ============================================================================
int1 sensors_is_tjunction(int8 pattern) {
    return (pattern == 0b101);
}

// ============================================================================
// CHECK IF LINE IS LOST (all sensors on white)
// ============================================================================
int1 sensors_is_lost(int8 pattern) {
    return (pattern == 0b000);
}

// ============================================================================
// LED CONTROL - STATUS
// ============================================================================
void sensors_led_status(int1 state) {
    if (state) {
        output_high(LED_STATUS);
    } else {
        output_low(LED_STATUS);
    }
}

// ============================================================================
// LED CONTROL - ACTION
// ============================================================================
void sensors_led_action(int1 state) {
    if (state) {
        output_high(LED_ACTION);
    } else {
        output_low(LED_ACTION);
    }
}

// ============================================================================
// LED BLINK ACTION (non-blocking, called from ISR)
// Toggles every 250ms
// ============================================================================
void sensors_led_blink_action(void) {
    if ((ms_tick - led_blink_start) >= 250) {
        led_blink_state = !led_blink_state;
        sensors_led_action(led_blink_state);
        led_blink_start = ms_tick;
    }
}

// ============================================================================
// CALIBRATION FUNCTIONS (optional - can be implemented if needed)
// ============================================================================
void sensors_calibrate_start(void) {
    // Implement auto-calibration if needed
    // For now, just mark as calibrated
    calibrated = TRUE;
}

void sensors_calibrate_update(void) {
    // Update calibration values
    // Not implemented - sensors work with digital threshold
}

int1 sensors_is_calibrated(void) {
    return calibrated;
}
