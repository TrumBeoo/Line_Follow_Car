////////////////////////////////////////////////////////////////////////////////
// FILE: sensors.h
// DESC: IR sensor reading and chatter filter header
////////////////////////////////////////////////////////////////////////////////

#ifndef SENSORS_H
#define SENSORS_H

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// Read raw sensor pattern (3 bits: Left-Mid-Right)
// Returns 0b000 to 0b111
int8 sensors_read_raw(void);

// Read filtered sensor pattern using chatter filter
// Requires N consecutive identical readings
int8 sensors_read_filtered(void);

// Chatter filter - called from ISR
// Updates internal state with new reading
void sensors_chatter_filter(int8 new_reading);

// Get last valid pattern (used when line is lost)
int8 sensors_get_last_valid(void);

// Set last valid pattern (for manual override)
void sensors_set_last_valid(int8 pattern);

// Check if pattern is valid line (not 000 or 111)
int1 sensors_is_valid_line(int8 pattern);

// Check if pattern indicates intersection (111)
int1 sensors_is_intersection(int8 pattern);

// Check if pattern indicates T-junction (101)
int1 sensors_is_tjunction(int8 pattern);

// Check if line is lost (000)
int1 sensors_is_lost(int8 pattern);

// LED control functions (for status indication)
void sensors_led_status(int1 state);
void sensors_led_action(int1 state);
void sensors_led_blink_action(void);  // Non-blocking blink

// Calibration functions (if needed)
void sensors_calibrate_start(void);
void sensors_calibrate_update(void);
int1 sensors_is_calibrated(void);

// Initialize sensor GPIO
void sensors_init(void);

#endif // SENSORS_H
