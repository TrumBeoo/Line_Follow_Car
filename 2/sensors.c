// ============================================================================
// FILE: sensors.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: IR sensor reading and chatter filter implementation
// ============================================================================

#include "sensors.h"
#include "main.h"

// ============================================================================
// CHATTER FILTER STATE
// ============================================================================

static uint8_t filter_history[CHATTER_N];
static uint8_t filter_index = 0;
static uint8_t filter_count = 0;
static uint8_t last_stable_pattern = PATTERN_CENTER;

// ============================================================================
// SENSOR FUNCTIONS
// ============================================================================

/**
 * Read raw sensor pattern
 * Note: Sensors are active-LOW (black line = 0), so we invert the reading
 * Returns: 3-bit pattern [LEFT][MIDDLE][RIGHT] where 1 = black line
 */
uint8_t read_sensor_pattern(void) {
    uint8_t pattern = 0;
    
    // Read sensors (active-low, so invert)
    if(!input(SENSOR_LEFT))   pattern |= 0b100;  // Bit 2
    if(!input(SENSOR_MIDDLE)) pattern |= 0b010;  // Bit 1
    if(!input(SENSOR_RIGHT))  pattern |= 0b001;  // Bit 0
    
    return pattern;
}

/**
 * Chatter filter - requires N consecutive identical readings
 * This eliminates sensor noise/spikes < N sample periods
 * @param new_pattern: New sensor reading
 * @return: Filtered stable pattern
 */
uint8_t chatter_filter(uint8_t new_pattern) {
    // Add new reading to history
    filter_history[filter_index] = new_pattern;
    filter_index = (filter_index + 1) % CHATTER_N;
    
    // Count samples (until buffer is full)
    if(filter_count < CHATTER_N) {
        filter_count++;
        return last_stable_pattern;  // Return last stable until buffer full
    }
    
    // Check if all N samples are identical
    uint8_t first = filter_history[0];
    bool all_same = true;
    
    for(uint8_t i = 1; i < CHATTER_N; i++) {
        if(filter_history[i] != first) {
            all_same = false;
            break;
        }
    }
    
    // If all samples match, update stable pattern
    if(all_same) {
        last_stable_pattern = first;
    }
    
    return last_stable_pattern;
}

/**
 * Check if pattern is valid (not lost line)
 */
bool is_valid_pattern(uint8_t pattern) {
    return (pattern != PATTERN_LOST);
}

/**
 * Check if pattern indicates intersection (all sensors on black)
 */
bool is_intersection(uint8_t pattern) {
    return (pattern == PATTERN_INTER);
}

/**
 * Check if pattern indicates T-junction
 */
bool is_t_junction(uint8_t pattern) {
    return (pattern == PATTERN_T_JUNC || pattern == PATTERN_INTER);
}

// ============================================================================
// LED CONTROL FUNCTIONS
// ============================================================================

void led_status_on(void) {
    output_high(LED_STATUS);
}

void led_status_off(void) {
    output_low(LED_STATUS);
}

void led_status_toggle(void) {
    output_toggle(LED_STATUS);
}

void led_action_on(void) {
    output_high(LED_ACTION);
}

void led_action_off(void) {
    output_low(LED_ACTION);
}

void led_action_toggle(void) {
    output_toggle(LED_ACTION);
}

// ============================================================================
// CALIBRATION (Optional - can implement auto-calibration)
// ============================================================================

/**
 * Simple calibration routine
 * Can be enhanced to auto-sample white/black thresholds
 */
void sensor_calibrate(void) {
    // Reset filter
    filter_index = 0;
    filter_count = 0;
    last_stable_pattern = PATTERN_CENTER;
    
    for(uint8_t i = 0; i < CHATTER_N; i++) {
        filter_history[i] = PATTERN_CENTER;
    }
    
    // Blink LED to indicate calibration
    for(uint8_t i = 0; i < 4; i++) {
        led_status_on();
        delay_ms(250);
        led_status_off();
        delay_ms(250);
    }
}