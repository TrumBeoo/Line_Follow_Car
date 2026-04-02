////////////////////////////////////////////////////////////////////////////////
// FILE: config.h
// DESC: Hardware configuration, fuses, and pin mapping for line follower robot
// DEVICE: PIC18F2685
// FREQ: 20MHz Crystal (HS mode)
////////////////////////////////////////////////////////////////////////////////

#ifndef CONFIG_H
#define CONFIG_H

#include <18F2685.h>

// ============================================================================
// FUSES CONFIGURATION
// ============================================================================
#fuses HS                    // High-speed crystal oscillator (20MHz)
#fuses NOWDT                 // No watchdog timer
#fuses NOPUT                 // No power-up timer
#fuses NOBROWNOUT            // No brownout reset
#fuses NOLVP                 // No low voltage programming
#fuses NOCPD                 // No data EEPROM protection
#fuses NOWRT                 // Program memory not write protected
#fuses NODEBUG               // No debug mode
#fuses NOPROTECT             // Code not protected
#fuses MCLR                  // Master clear enabled
#fuses NOPBADEN              // PORTB A/D pins configured as digital

#use delay(clock=20000000)   // 20MHz clock

// ============================================================================
// PIN MAPPING
// ============================================================================

// --- IR SENSORS (Line detection, Active LOW) ---
#define SENSOR_LEFT      PIN_A0    // Left IR sensor
#define SENSOR_MID       PIN_A1    // Middle IR sensor
#define SENSOR_RIGHT     PIN_A2    // Right IR sensor

// --- MOTOR DRIVER TB6612FNG ---
#define MOTOR_PWMA       PIN_C2    // PWM for Motor A (Left) - Software PWM
#define MOTOR_PWMB       PIN_C3    // PWM for Motor B (Right) - Software PWM
#define MOTOR_AIN1       PIN_C6    // Direction control A1 (Left motor - forward)
#define MOTOR_AIN2       PIN_A3    // Direction control A2 (Left motor - reverse)
#define MOTOR_BIN1       PIN_C7    // Direction control B1 (Right motor - forward)
#define MOTOR_BIN2       PIN_A4    // Direction control B2 (Right motor - reverse)
#define MOTOR_STBY       PIN_B6    // Standby (must be HIGH to enable)

// --- ULTRASONIC SENSOR HC-SR04 ---
#define ULTRA_TRIG       PIN_B4    // Trigger pin
#define ULTRA_ECHO       PIN_B5    // Echo pin

// --- PERIPHERALS ---
#define LED_STATUS       PIN_B2    // LED1 - Status LED (run indicator)
#define LED_ACTION       PIN_B3    // LED2 - Action LED
#define BUTTON_SYSTEM    PIN_B7    // System ON/OFF button (toggle)
#define BUTTON_STOP      PIN_B0    // STOP button (checkpoint logic)
#define BUTTON_RUN       PIN_B1    // RUN button (start from checkpoint)

// ============================================================================
// PWM SETTINGS
// ============================================================================
#define BASE_PWM         160       // Base PWM duty cycle (0-255)
#define SLOW_PWM         80        // Slow speed for precise maneuvers
#define STOP_PWM         0         // Stop motor

// ============================================================================
// SENSOR CONSTANTS
// ============================================================================
#define CHATTER_N        3         // Consecutive samples required for filter

// ============================================================================
// ULTRASONIC CONSTANTS
// ============================================================================
#define ULTRA_TIMEOUT    30000     // Timeout in microseconds (~500cm)
#define ULTRA_ERROR      255       // Error return value

// --- Distance Thresholds (cm) ---
#define STOP_CM_STATION  2         // Stop distance at ball pickup station
#define STOP_CM_END      5         // Stop distance at END point
#define REVERSE_MM       250       // Reverse distance after ball pickup (mm)

// ============================================================================
// TIMING CONSTANTS (ms)
// ============================================================================
#define LINE_LOST_MS     300       // Max time to maintain direction when line lost
#define NAV_MIN_MS       200       // Minimum time for navigation turn
#define CROSS_DELAY_MS   150       // Delay to cross intersection

// ============================================================================
// RETRY LIMITS
// ============================================================================
#define MAX_RETRIES      3         // Maximum error recovery attempts

// ============================================================================
// SENSOR PATTERN DEFINITIONS (3-bit: Left | Mid | Right)
// ============================================================================
#define PATTERN_000      0b000     // No line detected (all white)
#define PATTERN_001      0b001     // Strong right deviation
#define PATTERN_010      0b010     // Centered on line
#define PATTERN_011      0b011     // Light right deviation
#define PATTERN_100      0b100     // Strong left deviation
#define PATTERN_101      0b101     // T-junction or end point
#define PATTERN_110      0b110     // Light left deviation
#define PATTERN_111      0b111     // Intersection detected

// ============================================================================
// MOTOR DIRECTION DEFINITIONS
// ============================================================================
#define DIR_FORWARD      1
#define DIR_REVERSE      0

#endif // CONFIG_H