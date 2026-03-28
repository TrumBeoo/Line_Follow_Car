// ============================================================================
// FILE: config.h
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: Hardware configuration, fuses, and pin definitions
// ============================================================================

#ifndef CONFIG_H
#define CONFIG_H

// Device configuration
#device PIC18F2685
#include <18F2685.h>

// ============================================================================
// FUSES CONFIGURATION
// ============================================================================
#fuses HS              // High Speed Crystal (20MHz)
#fuses NOWDT           // Watchdog Timer OFF
#fuses NOPUT           // Power Up Timer OFF  
#fuses NOBROWNOUT      // Brown Out Reset OFF
#fuses NOLVP           // Low Voltage Programming OFF
#fuses NOCPD           // Code Protection OFF
#fuses NOWRT           // Flash Write Protection OFF
#fuses NODEBUG         // Background debugger OFF
#fuses NOPROTECT       // Code protection OFF
#fuses NOFCMEN         // Fail-safe clock monitor OFF
#fuses NOIESO          // Internal External Switchover OFF
#fuses NOPBADEN        // PORTB A/D Enable OFF (digital mode)
#fuses NOMCLR          // MCLR pin OFF (use as input)

// Clock configuration
#use delay(clock=20MHz, crystal=20MHz)

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// IR Sensors (Active LOW - black line = 0, white = 1)
#define SENSOR_LEFT    PIN_A0    // SL
#define SENSOR_MIDDLE  PIN_A1    // SM  
#define SENSOR_RIGHT   PIN_A2    // SR

// TB6612 Motor Driver
#define PWMA_PIN       PIN_C2    // PWM for Motor A (CCP1)
#define PWMB_PIN       PIN_C3    // PWM for Motor B (CCP2, mapped from C1)
#define AIN1_PIN       PIN_B0    // Motor A direction
#define AIN2_PIN       GND       // Fixed to GND
#define BIN1_PIN       PIN_B2    // Motor B direction  
#define BIN2_PIN       GND       // Fixed to GND
#define STBY_PIN       PIN_B6    // Standby control (HIGH = active)

// HC-SR04 Ultrasonic Sensor
#define TRIG_PIN       PIN_B4    // Trigger
#define ECHO_PIN       PIN_B5    // Echo

// Ball Mechanism
#define SERVO_GRAB_PIN PIN_C4    // Servo for grabbing ball
#define DROP_PIN       PIN_C5    // Drop ball mechanism

// LEDs
#define LED_STATUS     PIN_C0    // Status LED (calibration/error)
#define LED_ACTION     PIN_C1    // Action LED (ball operations)

// Buttons
#define BTN_RUN        PIN_B7    // RUN button (with IOCB interrupt)
#define BTN_STOP       PIN_B3    // STOP button / Ball sensor

// ============================================================================
// SYSTEM CONSTANTS  
// ============================================================================

// Motor PWM values (8-bit: 0-255)
#define BASE_PWM       160       // Base speed ~63% (for normal following)
#define TURN_PWM       120       // Turn speed ~47%
#define SLOW_PWM       100       // Slow speed ~39%  
#define BACK_PWM       120       // Reverse speed ~47%
#define STOP_PWM       0         // Stop

// Sensor patterns (3-bit: SL-SM-SR, inverted for active-low)
#define PATTERN_CENTER     0b010  // On line center
#define PATTERN_SLIGHT_L   0b110  // Slight left
#define PATTERN_SLIGHT_R   0b011  // Slight right
#define PATTERN_HARD_L     0b100  // Hard left
#define PATTERN_HARD_R     0b001  // Hard right
#define PATTERN_LOST       0b000  // Line lost
#define PATTERN_INTER      0b111  // Intersection
#define PATTERN_T_JUNC     0b101  // T-junction (can be treated as intersection)

// Chatter filter
#define CHATTER_N      3         // Number of consecutive samples required

// Ultrasonic thresholds (cm)
#define STOP_CM_STATION    2     // Stop distance at station
#define STOP_CM_END        5     // Stop distance at END point
#define TIMEOUT_CM         255   // Timeout return value

// Timing constants (milliseconds)
#define GRAB_MS            2000  // Grab ball duration
#define DROP_MS            2000  // Drop ball duration  
#define BACK_DELAY_MS      250   // Backup distance estimation (250mm ≈ 0.5s at BACK_PWM)
#define CROSS_DELAY_MS     150   // Time to cross intersection straight
#define TURN_DELAY_MS      200   // Minimum turn duration before checking sensor
#define LOST_TIMEOUT_MS    500   // Max time to maintain direction when line lost
#define DEBOUNCE_MS        50    // Button debounce time

// Checkpoint management
#define NUM_CHECKPOINTS    3     // Total number of checkpoints

// Timer configurations
#define TIMER0_PERIOD      2     // Timer0 interrupt period (ms)

#endif // CONFIG_H