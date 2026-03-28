////////////////////////////////////////////////////////////////////////////////
// FILE: ultrasonic.c
// DESC: Ultrasonic HC-SR04 sensor implementation
////////////////////////////////////////////////////////////////////////////////

#include "main.h"

// ============================================================================
// ULTRASONIC INITIALIZATION
// ============================================================================
void ultrasonic_init(void) {
    // Configure pins
    output_low(ULTRA_TRIG);    // Trigger pin as output, initial low
}

// ============================================================================
// TRIGGER PULSE GENERATION
// Generate 10us pulse on trigger pin
// ============================================================================
void ultrasonic_trigger(void) {
    output_low(ULTRA_TRIG);
    delay_us(2);
    output_high(ULTRA_TRIG);
    delay_us(10);
    output_low(ULTRA_TRIG);
}

// ============================================================================
// MEASURE ECHO PULSE WIDTH
// Returns pulse width in microseconds
// Timer1 runs continuously, just reset when measuring
// ============================================================================
int16 ultrasonic_measure_pulse(void) {
    int32 timeout_counter = 0;
    int16 pulse_width = 0;
    
    // Wait for echo to go high (with timeout)
    timeout_counter = 0;
    while (!input(ULTRA_ECHO)) {
        delay_us(1);
        timeout_counter++;
        if (timeout_counter > ULTRA_TIMEOUT) {
            return 0;  // Timeout - no echo
        }
    }
    
    // Reset Timer1 to measure pulse width
    // Timer1 already running from timers_init()
    set_timer1(0);
    
    // Wait for echo to go low (with timeout)
    timeout_counter = 0;
    while (input(ULTRA_ECHO)) {
        delay_us(1);
        timeout_counter++;
        if (timeout_counter > ULTRA_TIMEOUT) {
            return 0;  // Timeout - echo stuck high
        }
    }
    
    // Read timer value
    int16 ticks = get_timer1();
    
    // Convert ticks to microseconds
    // At 20MHz with DIV_BY_1: 1 tick = 0.2us
    // So: us = ticks * 0.2 = ticks / 5
    pulse_width = ticks / 5;
    
    return pulse_width;
}

// ============================================================================
// READ DISTANCE IN CENTIMETERS
// Speed of sound = 340 m/s = 29.4 us/cm (round trip)
// Distance (cm) = pulse_width (us) / 58
// ============================================================================
int8 ultrasonic_read_cm(void) {
    ultrasonic_trigger();
    delay_us(50);  // Small delay before measurement
    
    int16 pulse_us = ultrasonic_measure_pulse();
    
    if (pulse_us == 0) {
        return ULTRA_ERROR;  // Timeout or error
    }
    
    // Calculate distance in cm
    int16 distance_cm = pulse_us / 58;
    
    // Limit to 8-bit range
    if (distance_cm > 254) {
        return 254;
    }
    
    return (int8)distance_cm;
}

// ============================================================================
// READ DISTANCE IN MILLIMETERS (for precise reverse operation)
// Distance (mm) = pulse_width (us) / 5.8
// ============================================================================
int16 ultrasonic_read_mm(void) {
    ultrasonic_trigger();
    delay_us(50);
    
    int16 pulse_us = ultrasonic_measure_pulse();
    
    if (pulse_us == 0) {
        return 0xFFFF;  // Error value
    }
    
    // Calculate distance in mm (using fixed-point math)
    // mm = (pulse_us * 10) / 58
    int32 distance_mm = ((int32)pulse_us * 10) / 58;
    
    return (int16)distance_mm;
}

// ============================================================================
// CHECK IF OBJECT IS CLOSER THAN THRESHOLD
// ============================================================================
int1 ultrasonic_check_close(int8 threshold_cm) {
    int8 distance = ultrasonic_read_cm();
    
    if (distance == ULTRA_ERROR) {
        return FALSE;  // Error - assume not close
    }
    
    return (distance <= threshold_cm);
}
