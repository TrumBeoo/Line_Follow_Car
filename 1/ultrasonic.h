////////////////////////////////////////////////////////////////////////////////
// FILE: ultrasonic.h
// DESC: Ultrasonic sensor HC-SR04 header
////////////////////////////////////////////////////////////////////////////////

#ifndef ULTRASONIC_H
#define ULTRASONIC_H

// ============================================================================
// CONSTANTS
// ============================================================================
#define ULTRA_TIMEOUT    30000     // Timeout in microseconds (~500cm)
#define ULTRA_ERROR      255       // Error return value

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// Initialize ultrasonic sensor pins
void ultrasonic_init(void);

// Read distance in centimeters
// Returns distance in cm or ULTRA_ERROR on timeout
int8 ultrasonic_read_cm(void);

// Read distance in millimeters (for precise reverse operation)
int16 ultrasonic_read_mm(void);

// Check if object is closer than threshold
int1 ultrasonic_check_close(int8 threshold_cm);

// Trigger pulse generation
void ultrasonic_trigger(void);

// Wait for echo and calculate distance
// Low-level function used by read_cm()
int16 ultrasonic_measure_pulse(void);

#endif // ULTRASONIC_H
