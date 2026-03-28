// ============================================================================
// FILE: ultrasonic.h
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: HC-SR04 ultrasonic sensor declarations
// ============================================================================

#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// Read distance in centimeters using HC-SR04
// Returns: distance in cm, or TIMEOUT_CM (255) if timeout
uint8_t hcsr04_read_cm(void);

// Helper functions
void ultrasonic_trigger(void);
uint16_t ultrasonic_get_pulse_width(void);

// Check if object is within threshold
bool is_within_distance(uint8_t threshold_cm);

#endif // ULTRASONIC_H