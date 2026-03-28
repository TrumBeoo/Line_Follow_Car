////////////////////////////////////////////////////////////////////////////////
// FILE: test_modules.c
// DESC: Test program for individual modules (for debugging)
// NOTE: This is a separate test file, not part of main program
////////////////////////////////////////////////////////////////////////////////

#include "main.h"

// Comment/uncomment to select test mode
//#define TEST_SENSORS
//#define TEST_MOTOR
//#define TEST_ULTRASONIC
#define TEST_FULL_INTEGRATION

// ============================================================================
// TEST: SENSORS
// ============================================================================
#ifdef TEST_SENSORS
void test_sensors(void) {
    while (TRUE) {
        int8 raw = sensors_read_raw();
        int8 filtered = sensors_read_filtered();
        
        // Display on LEDs or UART
        printf("Raw: %03b  Filtered: %03b  Last: %03b\r\n", 
               raw, filtered, sensors_get_last_valid());
        
        // Visual feedback on LEDs
        if (sensors_is_lost(filtered)) {
            sensors_led_status(FALSE);
        } else {
            sensors_led_status(TRUE);
        }
        
        if (sensors_is_intersection(filtered)) {
            sensors_led_action(TRUE);
        } else {
            sensors_led_action(FALSE);
        }
        
        delay_ms(100);
    }
}
#endif

// ============================================================================
// TEST: MOTOR
// ============================================================================
#ifdef TEST_MOTOR
void test_motor(void) {
    motor_enable();
    
    while (TRUE) {
        // Test forward
        printf("Forward\r\n");
        motor_forward(BASE_PWM);
        delay_ms(2000);
        motor_stop();
        delay_ms(500);
        
        // Test reverse
        printf("Reverse\r\n");
        motor_reverse(BASE_PWM);
        delay_ms(2000);
        motor_stop();
        delay_ms(500);
        
        // Test turn right
        printf("Turn Right\r\n");
        motor_turn_right(BASE_PWM);
        delay_ms(1000);
        motor_stop();
        delay_ms(500);
        
        // Test turn left
        printf("Turn Left\r\n");
        motor_turn_left(BASE_PWM);
        delay_ms(1000);
        motor_stop();
        delay_ms(500);
        
        // Test pivot right
        printf("Pivot Right\r\n");
        motor_pivot_right(BASE_PWM);
        delay_ms(1000);
        motor_stop();
        delay_ms(500);
        
        // Test pivot left
        printf("Pivot Left\r\n");
        motor_pivot_left(BASE_PWM);
        delay_ms(1000);
        motor_stop();
        delay_ms(2000);
    }
}
#endif

// ============================================================================
// TEST: ULTRASONIC
// ============================================================================
#ifdef TEST_ULTRASONIC
void test_ultrasonic(void) {
    while (TRUE) {
        int8 distance_cm = ultrasonic_read_cm();
        int16 distance_mm = ultrasonic_read_mm();
        
        if (distance_cm == ULTRA_ERROR) {
            printf("Error: No echo\r\n");
            sensors_led_status(FALSE);
        } else {
            printf("Distance: %u cm (%lu mm)\r\n", distance_cm, distance_mm);
            sensors_led_status(TRUE);
        }
        
        // Visual indicator for close objects
        if (distance_cm <= STOP_CM_STATION) {
            sensors_led_action(TRUE);
        } else {
            sensors_led_action(FALSE);
        }
        
        delay_ms(500);
    }
}
#endif

// ============================================================================
// TEST: FULL INTEGRATION
// ============================================================================
#ifdef TEST_FULL_INTEGRATION
void test_integration(void) {
    printf("=== FULL SYSTEM TEST ===\r\n");
    
    // Test 1: Sensor reading
    printf("\n1. Testing sensors...\r\n");
    for (int i = 0; i < 10; i++) {
        int8 pattern = sensors_read_filtered();
        printf("  Pattern: %03b\r\n", pattern);
        delay_ms(200);
    }
    
    // Test 2: Motor lookup table
    printf("\n2. Testing motor lookup table...\r\n");
    motor_enable();
    for (int8 pattern = 0; pattern <= 7; pattern++) {
        MotorCmd_t cmd = motor_get_command(pattern);
        printf("  Pattern %03b: L=%u R=%u Dir=%u\r\n", 
               pattern, cmd.left_pwm, cmd.right_pwm, cmd.direction);
        motor_set(cmd.left_pwm, cmd.right_pwm, cmd.direction);
        delay_ms(500);
        motor_stop();
        delay_ms(300);
    }
    
    // Test 3: Ultrasonic
    printf("\n3. Testing ultrasonic...\r\n");
    for (int i = 0; i < 5; i++) {
        int8 dist = ultrasonic_read_cm();
        printf("  Distance: %u cm\r\n", dist);
        delay_ms(500);
    }
    
    // Test 4: FSM transitions
    printf("\n4. Testing FSM...\r\n");
    printf("  Initial state: %u\r\n", current_state);
    
    fsm_transition(STATE_FOLLOW_LINE);
    printf("  After transition: %u\r\n", current_state);
    delay_ms(1000);
    
    fsm_transition(STATE_IDLE);
    printf("  Back to IDLE: %u\r\n", current_state);
    
    // Test 5: Checkpoint save/restore
    printf("\n5. Testing checkpoint...\r\n");
    fsm_save_checkpoint(CP_AFTER_STATION);
    printf("  Saved checkpoint: %u\r\n", current_checkpoint);
    
    delay_ms(500);
    Checkpoint_t loaded = read_eeprom(EEPROM_CHECKPOINT);
    printf("  Loaded checkpoint: %u\r\n", loaded);
    
    printf("\n=== ALL TESTS COMPLETE ===\r\n");
    
    // Flash LEDs
    for (int i = 0; i < 5; i++) {
        sensors_led_status(TRUE);
        sensors_led_action(TRUE);
        delay_ms(200);
        sensors_led_status(FALSE);
        sensors_led_action(FALSE);
        delay_ms(200);
    }
}
#endif

// ============================================================================
// MAIN FOR TESTING
// ============================================================================
void main(void) {
    // Initialize system
    system_init();
    
    #ifdef TEST_SENSORS
    test_sensors();
    #endif
    
    #ifdef TEST_MOTOR
    test_motor();
    #endif
    
    #ifdef TEST_ULTRASONIC
    test_ultrasonic();
    #endif
    
    #ifdef TEST_FULL_INTEGRATION
    test_integration();
    #endif
    
    // If no test selected, run normal program
    while (TRUE) {
        sensors_led_status(TRUE);
        delay_ms(500);
        sensors_led_status(FALSE);
        delay_ms(500);
    }
}
