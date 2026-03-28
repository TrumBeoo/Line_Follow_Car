// ============================================================================
// FILE: test_modules.c
// PROJECT: Line Follower Robot - PIC18F2685
// DESCRIPTION: Module testing suite - test each module independently
// ============================================================================

#include "main.h"

// ============================================================================
// TEST SELECTION - Comment/uncomment to select test
// ============================================================================
#define TEST_MOTOR          // Test motor control
//#define TEST_SENSORS        // Test IR sensors and chatter filter
//#define TEST_ULTRASONIC     // Test HC-SR04 ultrasonic
//#define TEST_LEDS           // Test LED indicators
//#define TEST_BUTTONS        // Test button inputs
//#define TEST_PWM            // Test PWM generation
//#define TEST_TIMER          // Test Timer0 interrupt
//#define TEST_LOOKUP_TABLE   // Test motor lookup table

// ============================================================================
// GLOBAL VARIABLES (minimal for testing)
// ============================================================================
volatile uint32_t ms_tick = 0;
volatile bool run_flag = false;
volatile bool stop_flag = false;

// ============================================================================
// BASIC INITIALIZATION
// ============================================================================

void test_gpio_init(void) {
    // Port A: Digital I/O for sensors
    ADCON1 = 0x0F;
    
    // Sensors as inputs
    output_float(SENSOR_LEFT);
    output_float(SENSOR_MIDDLE);
    output_float(SENSOR_RIGHT);
    
    // Motor pins as outputs
    output_low(AIN1_PIN);
    output_low(BIN1_PIN);
    output_low(STBY_PIN);
    
    // Ultrasonic pins
    output_low(TRIG_PIN);
    output_float(ECHO_PIN);
    
    // LEDs as outputs
    output_low(LED_STATUS);
    output_low(LED_ACTION);
    
    // Buttons as inputs with pull-ups
    port_b_pullups(0b10001000);  // RB7, RB3
}

void test_timer_init(void) {
    // Timer0: System tick (2ms)
    setup_timer_0(RTCC_INTERNAL | RTCC_DIV_256);
    set_timer0(65497);
    enable_interrupts(INT_TIMER0);
    
    // Timer1: Ultrasonic measurement
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_1);
    set_timer1(0);
    
    enable_interrupts(GLOBAL);
}

void test_pwm_init(void) {
    setup_ccp1(CCP_PWM);
    setup_ccp2(CCP_PWM);
    setup_timer_2(T2_DIV_BY_1, 249, 1);  // 20kHz PWM
}

// ============================================================================
// TIMER0 ISR (for timing tests)
// ============================================================================
#INT_TIMER0
void timer0_isr(void) {
    set_timer0(65497);  // Reload for 2ms @ 20MHz with 1:256 prescaler
    ms_tick += 2;
}

// ============================================================================
// TEST 1: MOTOR CONTROL
// ============================================================================
#ifdef TEST_MOTOR

void test_motor(void) {
    fprintf(UART, "\r\n=== MOTOR TEST ===\r\n");
    
    motor_enable();
    delay_ms(500);
    
    // Test 1: Forward
    fprintf(UART, "Forward BASE_PWM...\r\n");
    motor_forward(BASE_PWM);
    delay_ms(2000);
    motor_stop();
    delay_ms(1000);
    
    // Test 2: Reverse
    fprintf(UART, "Reverse BACK_PWM...\r\n");
    motor_reverse(BACK_PWM);
    delay_ms(2000);
    motor_stop();
    delay_ms(1000);
    
    // Test 3: Turn left
    fprintf(UART, "Turn left...\r\n");
    motor_turn_left(TURN_PWM, BASE_PWM);
    delay_ms(2000);
    motor_stop();
    delay_ms(1000);
    
    // Test 4: Turn right
    fprintf(UART, "Turn right...\r\n");
    motor_turn_right(BASE_PWM, TURN_PWM);
    delay_ms(2000);
    motor_stop();
    delay_ms(1000);
    
    // Test 5: Pivot left
    fprintf(UART, "Pivot left...\r\n");
    motor_pivot_left(TURN_PWM);
    delay_ms(1500);
    motor_stop();
    delay_ms(1000);
    
    // Test 6: Pivot right
    fprintf(UART, "Pivot right...\r\n");
    motor_pivot_right(TURN_PWM);
    delay_ms(1500);
    motor_stop();
    delay_ms(1000);
    
    motor_disable();
    fprintf(UART, "Motor test complete!\r\n");
}

#endif

// ============================================================================
// TEST 2: SENSORS
// ============================================================================
#ifdef TEST_SENSORS

void test_sensors(void) {
    fprintf(UART, "\r\n=== SENSOR TEST ===\r\n");
    fprintf(UART, "Reading sensors for 30 seconds...\r\n");
    fprintf(UART, "Format: [L M R] Pattern Filtered\r\n\n");
    
    uint32_t start_time = ms_tick;
    
    while((ms_tick - start_time) < 30000) {
        uint8_t raw = read_sensor_pattern();
        uint8_t filtered = chatter_filter(raw);
        
        fprintf(UART, "[%d %d %d] 0x%02X -> 0x%02X\r\n",
                (raw & 0b100) ? 1 : 0,
                (raw & 0b010) ? 1 : 0,
                (raw & 0b001) ? 1 : 0,
                raw, filtered);
        
        // LED feedback
        if(filtered == PATTERN_CENTER) {
            led_status_on();
        } else {
            led_status_off();
        }
        
        delay_ms(200);
    }
    
    fprintf(UART, "Sensor test complete!\r\n");
}

#endif

// ============================================================================
// TEST 3: ULTRASONIC
// ============================================================================
#ifdef TEST_ULTRASONIC

void test_ultrasonic(void) {
    fprintf(UART, "\r\n=== ULTRASONIC TEST ===\r\n");
    fprintf(UART, "Reading distance for 30 seconds...\r\n\n");
    
    uint32_t start_time = ms_tick;
    
    while((ms_tick - start_time) < 30000) {
        uint8_t distance = hcsr04_read_cm();
        
        if(distance == TIMEOUT_CM) {
            fprintf(UART, "Distance: TIMEOUT\r\n");
        } else {
            fprintf(UART, "Distance: %u cm\r\n", distance);
        }
        
        // LED feedback for close objects
        if(distance <= 10 && distance != TIMEOUT_CM) {
            led_action_on();
        } else {
            led_action_off();
        }
        
        delay_ms(500);
    }
    
    fprintf(UART, "Ultrasonic test complete!\r\n");
}

#endif

// ============================================================================
// TEST 4: LEDS
// ============================================================================
#ifdef TEST_LEDS

void test_leds(void) {
    fprintf(UART, "\r\n=== LED TEST ===\r\n");
    
    // Test 1: Alternating blink
    fprintf(UART, "Alternating blink...\r\n");
    for(uint8_t i = 0; i < 10; i++) {
        led_status_on();
        led_action_off();
        delay_ms(300);
        led_status_off();
        led_action_on();
        delay_ms(300);
    }
    led_action_off();
    
    // Test 2: Simultaneous blink
    fprintf(UART, "Simultaneous blink...\r\n");
    for(uint8_t i = 0; i < 10; i++) {
        led_status_on();
        led_action_on();
        delay_ms(300);
        led_status_off();
        led_action_off();
        delay_ms(300);
    }
    
    // Test 3: Fast toggle
    fprintf(UART, "Fast toggle...\r\n");
    for(uint8_t i = 0; i < 20; i++) {
        led_status_toggle();
        led_action_toggle();
        delay_ms(100);
    }
    
    led_status_off();
    led_action_off();
    fprintf(UART, "LED test complete!\r\n");
}

#endif

// ============================================================================
// TEST 5: BUTTONS
// ============================================================================
#ifdef TEST_BUTTONS

void test_buttons(void) {
    fprintf(UART, "\r\n=== BUTTON TEST ===\r\n");
    fprintf(UART, "Press RUN (RB7) or STOP (RB3) buttons...\r\n");
    fprintf(UART, "Test runs for 30 seconds\r\n\n");
    
    uint32_t start_time = ms_tick;
    bool last_run = false;
    bool last_stop = false;
    
    while((ms_tick - start_time) < 30000) {
        bool run_pressed = !input(BTN_RUN);
        bool stop_pressed = !input(BTN_STOP);
        
        // Detect rising edge
        if(run_pressed && !last_run) {
            fprintf(UART, "RUN button pressed!\r\n");
            led_status_toggle();
        }
        
        if(stop_pressed && !last_stop) {
            fprintf(UART, "STOP button pressed!\r\n");
            led_action_toggle();
        }
        
        last_run = run_pressed;
        last_stop = stop_pressed;
        
        delay_ms(50);  // Debounce
    }
    
    led_status_off();
    led_action_off();
    fprintf(UART, "Button test complete!\r\n");
}

#endif

// ============================================================================
// TEST 6: PWM
// ============================================================================
#ifdef TEST_PWM

void test_pwm(void) {
    fprintf(UART, "\r\n=== PWM TEST ===\r\n");
    fprintf(UART, "Testing PWM duty cycles on CCP1 (RC2) and CCP2 (RC3)\r\n");
    
    motor_enable();
    output_high(AIN1_PIN);
    output_high(BIN1_PIN);
    
    // Test different duty cycles
    uint8_t duties[] = {0, 64, 128, 160, 192, 255};
    
    for(uint8_t i = 0; i < 6; i++) {
        fprintf(UART, "Duty: %u/255 (%u%%)\r\n", 
                duties[i], (uint16_t)duties[i] * 100 / 255);
        
        set_pwm1_duty(duties[i]);
        set_pwm2_duty(duties[i]);
        delay_ms(2000);
    }
    
    set_pwm1_duty(0);
    set_pwm2_duty(0);
    motor_disable();
    
    fprintf(UART, "PWM test complete!\r\n");
}

#endif

// ============================================================================
// TEST 7: TIMER
// ============================================================================
#ifdef TEST_TIMER

void test_timer(void) {
    fprintf(UART, "\r\n=== TIMER TEST ===\r\n");
    fprintf(UART, "Testing Timer0 2ms interrupt...\r\n");
    fprintf(UART, "LED should blink every 500ms\r\n\n");
    
    uint32_t last_blink = 0;
    uint32_t start_time = ms_tick;
    
    while((ms_tick - start_time) < 30000) {
        if((ms_tick - last_blink) >= 500) {
            led_status_toggle();
            last_blink = ms_tick;
            fprintf(UART, "Tick: %lu ms\r\n", ms_tick);
        }
    }
    
    led_status_off();
    fprintf(UART, "Timer test complete!\r\n");
}

#endif

// ============================================================================
// TEST 8: LOOKUP TABLE
// ============================================================================
#ifdef TEST_LOOKUP_TABLE

void test_lookup_table(void) {
    fprintf(UART, "\r\n=== LOOKUP TABLE TEST ===\r\n");
    fprintf(UART, "Testing all 8 sensor patterns...\r\n\n");
    
    motor_enable();
    
    for(uint8_t pattern = 0; pattern < 8; pattern++) {
        fprintf(UART, "Pattern 0b%03b: ", pattern);
        
        switch(pattern) {
            case 0b000: fprintf(UART, "LOST    "); break;
            case 0b001: fprintf(UART, "HARD_R  "); break;
            case 0b010: fprintf(UART, "CENTER  "); break;
            case 0b011: fprintf(UART, "SLIGHT_R"); break;
            case 0b100: fprintf(UART, "HARD_L  "); break;
            case 0b101: fprintf(UART, "T_JUNC  "); break;
            case 0b110: fprintf(UART, "SLIGHT_L"); break;
            case 0b111: fprintf(UART, "INTER   "); break;
        }
        
        const MotorCmd_t *cmd = &motor_table[pattern];
        fprintf(UART, " -> L:%u R:%u %s\r\n",
                cmd->left_speed, cmd->right_speed,
                cmd->forward ? "FWD" : "REV");
        
        motor_apply_table(pattern);
        delay_ms(2000);
        motor_stop();
        delay_ms(500);
    }
    
    motor_disable();
    fprintf(UART, "Lookup table test complete!\r\n");
}

#endif

// ============================================================================
// MAIN FUNCTION
// ============================================================================

void main(void) {
    // Basic initialization
    test_gpio_init();
    test_timer_init();
    test_pwm_init();
    
    delay_ms(1000);
    
    // Run selected test
    #ifdef TEST_MOTOR
        test_motor();
    #endif
    
    #ifdef TEST_SENSORS
        test_sensors();
    #endif
    
    #ifdef TEST_ULTRASONIC
        test_ultrasonic();
    #endif
    
    #ifdef TEST_LEDS
        test_leds();
    #endif
    
    #ifdef TEST_BUTTONS
        test_buttons();
    #endif
    
    #ifdef TEST_PWM
        test_pwm();
    #endif
    
    #ifdef TEST_TIMER
        test_timer();
    #endif
    
    #ifdef TEST_LOOKUP_TABLE
        test_lookup_table();
    #endif
    
    // Infinite loop with status LED
    while(true) {
        led_status_toggle();
        delay_ms(1000);
    }
}
