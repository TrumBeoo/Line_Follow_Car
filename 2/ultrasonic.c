// ============================================================================
// FILE: ultrasonic.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: HC-SR04 ultrasonic sensor implementation
// ============================================================================

#include "ultrasonic.h"
#include "main.h"

// ============================================================================
// ULTRASONIC MEASUREMENT FUNCTIONS
// ============================================================================

/**
 * Trigger ultrasonic sensor (send 10us pulse)
 */
void ultrasonic_trigger(void) {
    output_high(TRIG_PIN);
    delay_us(10);
    output_low(TRIG_PIN);
}

/**
 * Get echo pulse width in microseconds using Timer1
 * Timer1 runs at 5MHz (Fosc/4), so 1 tick = 0.2us
 * Timeout: ~25ms (max range ~4m)
 * @return: Pulse width in microseconds, or 0 if timeout
 */
uint16_t ultrasonic_get_pulse_width(void) {
    uint16_t timeout_counter = 0;
    uint16_t pulse_ticks = 0;
    
    // Wait for echo to go HIGH (start of pulse) - max 25ms
    timeout_counter = 0;
    while(!input(ECHO_PIN)) {
        delay_us(10);
        timeout_counter++;
        if(timeout_counter > 2500) {  // 25ms timeout
            return 0;  // Timeout - no echo received
        }
    }
    
    // Start Timer1 when echo goes HIGH
    set_timer1(0);
    
    // Wait for echo to go LOW and measure pulse width
    while(input(ECHO_PIN)) {
        pulse_ticks = get_timer1();
        // Check for timeout: 25ms = 125000 ticks at 5MHz
        if(pulse_ticks > 125000) {
            return 0;  // Timeout - pulse too long
        }
    }
    
    // Get final tick count
    pulse_ticks = get_timer1();
    
    // Convert ticks to microseconds
    // 1 tick = 0.2us, so pulse_us = pulse_ticks * 0.2 = pulse_ticks / 5
    return (pulse_ticks / 5);
}

/**
 * Read distance in centimeters
 * Formula: distance_cm = pulse_us / 58
 * (Sound speed = 340m/s, round trip, so 58us per cm)
 * @return: Distance in cm, or TIMEOUT_CM (255) if error/timeout
 */
uint8_t hcsr04_read_cm(void) {
    uint16_t pulse_us;
    uint16_t distance_cm;
    
    // Trigger measurement
    ultrasonic_trigger();
    
    // Get pulse width
    pulse_us = ultrasonic_get_pulse_width();
    
    // Check for timeout
    if(pulse_us == 0) {
        return TIMEOUT_CM;
    }
    
    // Calculate distance (pulse_us / 58)
    distance_cm = pulse_us / 58;
    
    // Limit to 8-bit range
    if(distance_cm > 254) {
        return TIMEOUT_CM;
    }
    
    return (uint8_t)distance_cm;
}

/**
 * Check if object is within threshold distance
 * @param threshold_cm: Distance threshold in centimeters
 * @return: true if object detected within threshold, false otherwise
 */
bool is_within_distance(uint8_t threshold_cm) {
    uint8_t distance = hcsr04_read_cm();
    
    // Check for valid reading (not timeout)
    if(distance == TIMEOUT_CM) {
        return false;
    }
    
    return (distance <= threshold_cm);
}