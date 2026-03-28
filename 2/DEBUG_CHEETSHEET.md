# 🐛 DEBUG CHEAT SHEET

## Quick Diagnostic Tests

### 1. Test IR Sensors Individually
```c
void test_sensors(void) {
    while(1) {
        uint8_t pattern = read_sensor_pattern();
        
        led_status_off();
        led_action_off();
        
        if(pattern & 0b100) led_status_on();  // Left sensor
        if(pattern & 0b010) led_action_on();  // Middle sensor  
        // Right sensor: Thêm LED thứ 3 hoặc dùng UART
        
        printf("Pattern: %03b (L=%d M=%d R=%d)\n",
               pattern,
               (pattern & 0b100) >> 2,
               (pattern & 0b010) >> 1,
               (pattern & 0b001));
        
        delay_ms(100);
    }
}
```

### 2. Test Motor Direction
```c
void test_motors(void) {
    motor_enable();
    
    // Test Motor A forward
    printf("Motor A Forward\n");
    motor_set(BASE_PWM, 0, DIR_FORWARD);
    delay_ms(2000);
    motor_stop();
    delay_ms(1000);
    
    // Test Motor A reverse
    printf("Motor A Reverse\n");
    motor_set(BASE_PWM, 0, DIR_REVERSE);
    delay_ms(2000);
    motor_stop();
    delay_ms(1000);
    
    // Test Motor B forward
    printf("Motor B Forward\n");
    motor_set(0, BASE_PWM, DIR_FORWARD);
    delay_ms(2000);
    motor_stop();
    delay_ms(1000);
    
    // Test Motor B reverse
    printf("Motor B Reverse\n");
    motor_set(0, BASE_PWM, DIR_REVERSE);
    delay_ms(2000);
    motor_stop();
    delay_ms(1000);
    
    // Test both forward
    printf("Both Forward\n");
    motor_forward(BASE_PWM);
    delay_ms(2000);
    motor_stop();
    
    motor_disable();
}
```

### 3. Test Ultrasonic Sensor
```c
void test_ultrasonic(void) {
    while(1) {
        uint8_t dist = hcsr04_read_cm();
        
        if(dist == TIMEOUT_CM) {
            printf("Distance: TIMEOUT\n");
            led_status_on();
        } else {
            printf("Distance: %u cm\n", dist);
            led_status_off();
            
            // Blink when object close
            if(dist < 10) {
                led_action_on();
            } else {
                led_action_off();
            }
        }
        
        delay_ms(200);
    }
}
```

### 4. Test PWM Output
```c
void test_pwm(void) {
    motor_enable();
    
    // Ramp PWM from 0 to 255
    for(uint16_t duty = 0; duty <= 255; duty += 5) {
        set_pwm1_duty(duty);
        set_pwm2_duty(duty);
        printf("PWM Duty: %u/255 (%u%%)\n", duty, (duty*100)/255);
        delay_ms(100);
    }
    
    // Ramp down
    for(int16_t duty = 255; duty >= 0; duty -= 5) {
        set_pwm1_duty(duty);
        set_pwm2_duty(duty);
        delay_ms(100);
    }
    
    motor_disable();
}
```

### 5. Test Chatter Filter
```c
void test_chatter_filter(void) {
    printf("Chatter Filter Test (inject noise)\n");
    
    uint8_t test_patterns[] = {
        0b010, 0b010, 0b010,  // Stable center
        0b011, 0b010, 0b011,  // Noise
        0b011, 0b011, 0b011,  // Stable right
        0b000, 0b011, 0b000,  // Spike
        0b000, 0b000, 0b000   // Stable lost
    };
    
    for(uint8_t i = 0; i < sizeof(test_patterns); i++) {
        uint8_t filtered = chatter_filter(test_patterns[i]);
        printf("Raw: %03b -> Filtered: %03b\n", 
               test_patterns[i], filtered);
        delay_ms(50);
    }
}
```

### 6. Test State Transitions
```c
void test_state_machine(void) {
    printf("FSM Transition Test\n");
    
    // Test each state entry/exit
    transition(STATE_IDLE);
    delay_ms(1000);
    
    transition(STATE_FOLLOW_LINE);
    delay_ms(1000);
    
    transition(STATE_STATION);
    delay_ms(1000);
    
    transition(STATE_IDLE);
    
    printf("All transitions OK\n");
}
```

### 7. Test Button Interrupts
```c
void test_buttons(void) {
    printf("Button Test - Press RUN and STOP\n");
    
    run_flag = false;
    stop_flag = false;
    
    while(1) {
        if(run_flag) {
            printf("RUN button pressed!\n");
            led_status_toggle();
            run_flag = false;
        }
        
        if(stop_flag) {
            printf("STOP button pressed!\n");
            led_action_toggle();
            stop_flag = false;
        }
        
        delay_ms(10);
    }
}
```

## 🔍 Visual Debug with LEDs

### LED Pattern Meanings
```c
void debug_indicate_state(void) {
    switch(current_state) {
        case STATE_IDLE:
            // 1 blink
            led_status_on(); delay_ms(100);
            led_status_off(); delay_ms(900);
            break;
            
        case STATE_FOLLOW_LINE:
            // Solid ON
            led_status_on();
            break;
            
        case STATE_STATION:
            // Fast blink
            led_status_toggle();
            delay_ms(100);
            break;
            
        case STATE_ERROR:
            // Both LEDs blink
            led_status_toggle();
            led_action_toggle();
            delay_ms(200);
            break;
    }
}
```

## 📊 Performance Monitoring

### Measure Loop Time
```c
void measure_loop_time(void) {
    static uint32_t last_time = 0;
    static uint32_t max_time = 0;
    static uint32_t min_time = 999999;
    
    uint32_t current_time = ms_tick;
    uint32_t loop_time = current_time - last_time;
    
    if(loop_time > max_time) max_time = loop_time;
    if(loop_time < min_time && loop_time > 0) min_time = loop_time;
    
    printf("Loop: %lums (min=%lu, max=%lu)\n", 
           loop_time, min_time, max_time);
    
    last_time = current_time;
}
```

### Monitor Stack Usage
```c
#define STACK_CANARY 0xA5A5

void check_stack_usage(void) {
    extern uint8_t _stack;
    extern uint8_t _stackend;
    
    uint16_t stack_size = &_stackend - &_stack;
    
    // Fill stack with canary pattern
    memset(&_stack, STACK_CANARY, stack_size);
    
    // After running, check how much is used
    uint16_t unused = 0;
    for(uint16_t i = 0; i < stack_size; i++) {
        if((&_stack)[i] == STACK_CANARY) {
            unused++;
        } else {
            break;
        }
    }
    
    uint16_t used = stack_size - unused;
    printf("Stack: %u/%u used (%u%%)\n", 
           used, stack_size, (used*100)/stack_size);
}
```

## 🚨 Error Code Meanings

### Blink Patterns for Errors
```
Pattern          | Error Type
-----------------|---------------------------
1 blink          | ERR_NONE (OK)
2 blinks         | ERR_NOISE (sensor noise)
3 blinks         | ERR_LOSTLINE (line lost)
4 blinks         | ERR_TIMEOUT (operation timeout)
5 blinks         | ERR_SENSOR_FAIL (hardware fail)
Continuous blink | CRITICAL ERROR
```

### Implement Error Blink
```c
void blink_error_code(ErrorType_t error) {
    uint8_t blinks = (uint8_t)error + 1;
    
    for(uint8_t i = 0; i < blinks; i++) {
        led_status_on();
        led_action_on();
        delay_ms(200);
        led_status_off();
        led_action_off();
        delay_ms(200);
    }
    
    delay_ms(1000);  // Pause between sequences
}
```

## 🔧 Quick Fixes

### Fix 1: Sensor Reading Inverted
```c
// If sensors read opposite (white=1, black=0)
uint8_t read_sensor_pattern(void) {
    uint8_t pattern = 0;
    
    // Original: Inverted (correct for active-LOW)
    if(!input(SENSOR_LEFT))   pattern |= 0b100;
    
    // If wrong, try:
    if(input(SENSOR_LEFT))    pattern |= 0b100;  // No inversion
}
```

### Fix 2: Motor Swapped
```c
// If left motor is actually right motor
void motor_set(uint8_t left_pwm, uint8_t right_pwm, MotorDir_t dir) {
    // Swap:
    set_pwm1_duty(right_pwm);  // Was left_pwm
    set_pwm2_duty(left_pwm);   // Was right_pwm
    
    // Also swap direction pins if needed
}
```

### Fix 3: Timer0 Wrong Period
```c
// Measure actual period
void calibrate_timer0(void) {
    uint32_t start = ms_tick;
    delay_ms(1000);  // Using built-in delay
    uint32_t elapsed = ms_tick - start;
    
    printf("Expected: 1000ms, Actual: %lums\n", elapsed);
    printf("Error: %ld%%\n", ((int32_t)elapsed - 1000) / 10);
    
    // Adjust TMR0 reload value accordingly
}
```

### Fix 4: Ultrasonic Always Timeout
```c
// Try slower trigger
void ultrasonic_trigger(void) {
    output_high(TRIG_PIN);
    delay_us(15);  // Try 15us instead of 10us
    output_low(TRIG_PIN);
}

// Or increase timeout
uint16_t ultrasonic_get_pulse_width(void) {
    // Increase timeout from 25000 to 50000
    while(!input(ECHO_PIN)) {
        delay_us(1);
        timeout_counter++;
        if(timeout_counter > 50000) return 0;
    }
}
```

## 📝 Debug Log Format

### Structured Logging
```c
#define LOG_ERROR(msg)   printf("[ERROR] %s\n", msg)
#define LOG_WARN(msg)    printf("[WARN]  %s\n", msg)
#define LOG_INFO(msg)    printf("[INFO]  %s\n", msg)
#define LOG_DEBUG(msg)   printf("[DEBUG] %s\n", msg)

// Usage:
LOG_INFO("System initialized");
LOG_WARN("Line lost, maintaining direction");
LOG_ERROR("Sensor failure detected");
```

### State Change Log
```c
void transition(SystemState_t new_state) {
    printf("[FSM] %s -> %s\n", 
           state_names[current_state],
           state_names[new_state]);
    
    // Rest of transition code...
}

const char* state_names[] = {
    "IDLE", "FOLLOW_LINE", "STATION", 
    "STATION_STOP", "STATION_BACK", "NAVIGATION",
    "END", "ERROR", "CHECKPOINT_1", "CHECKPOINT_2"
};
```

## 🎯 Troubleshooting Flowchart

```
Problem: Xe không hoạt động
│
├─> LEDs có sáng không?
│   ├─> Không → Kiểm tra nguồn, fuses
│   └─> Có → Continue
│
├─> Motor có quay không?
│   ├─> Không → Kiểm tra STBY pin, TB6612 power
│   └─> Có → Continue
│
├─> Sensor đọc được không?
│   ├─> Không → Kiểm tra ADCON1, sensor power
│   └─> Có → Continue
│
├─> Xe bám line không?
│   ├─> Không → Calibrate lookup table
│   └─> Có → Continue
│
└─> Xe không dừng tại station?
    └─> Calibrate ultrasonic threshold
```

---

**Pro Tip**: Luôn test từng module riêng lẻ trước khi tích hợp! 🎯