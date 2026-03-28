# GHI CHÚ KỸ THUẬT TRIỂN KHAI

## 🎯 Những điểm quan trọng cần lưu ý khi nhúng code

### 1. Cấu hình PWM cho CCP2

**Vấn đề**: PIC18F2685 có CCP2 multiplexed giữa RC1 và RB3 (tùy thuộc bit CCP2MX)

**Giải pháp**:
```c
// Trong config.h fuses, thêm:
#fuses CCP2C1  // Hoặc kiểm tra datasheet cho bit CCP2MX

// Nếu không thể map CCP2 sang RC3, có 3 options:
// Option 1: Dùng RC1 thay vì RC3 cho PWMB
// Option 2: Software PWM cho motor thứ 2
// Option 3: Dùng Enhanced PWM module (nếu có)
```

**Code sửa đổi nếu cần software PWM**:
```c
// Trong timer0_isr(), thêm software PWM:
static uint8_t pwm_counter = 0;
pwm_counter++;
if(pwm_counter >= motor_b_duty) {
    output_low(PWMB_PIN);
} else {
    output_high(PWMB_PIN);
}
if(pwm_counter >= 255) pwm_counter = 0;
```

### 2. Chatter Filter - Chi tiết hoạt động

**Tại sao cần**: Loại bỏ nhiễu < 3 chu kỳ (< 6ms với Timer0 2ms)

**Hoạt động**:
- Mảng 3 phần tử lưu 3 lần đọc gần nhất
- Chỉ chấp nhận pattern khi cả 3 lần đọc giống nhau
- Buffer chưa đầy → trả về last_stable_pattern

**Test chatter filter**:
```c
// Trong main(), trước while(1):
#ifdef DEBUG_CHATTER
for(uint8_t i = 0; i < 10; i++) {
    uint8_t raw = read_sensor_pattern();
    uint8_t filtered = chatter_filter(raw);
    printf("Raw: %03b, Filtered: %03b\n", raw, filtered);
    delay_ms(50);
}
#endif
```

### 3. Lookup Table vs PID

**Tại sao dùng Lookup Table**:
- Đơn giản, không cần tuning
- Phản hồi nhanh, không delay
- RAM thấp (8 entries × 3 bytes = 24 bytes)
- Phù hợp với đường đơn giản

**Khi nào cần PID**:
- Đường cong nhiều
- Tốc độ cao
- Yêu cầu smooth hơn
- Nhiều RAM và CPU

**Nâng cấp lên PID** (nếu cần):
```c
typedef struct {
    float Kp, Ki, Kd;
    float last_error;
    float integral;
} PID_t;

int16_t pid_compute(PID_t *pid, int8_t error) {
    pid->integral += error;
    float derivative = error - pid->last_error;
    float output = pid->Kp * error + 
                   pid->Ki * pid->integral + 
                   pid->Kd * derivative;
    pid->last_error = error;
    return (int16_t)output;
}
```

### 4. Ultrasonic Polling vs Interrupt

**Hiện tại**: Dùng polling (đơn giản, đủ dùng)

**Nâng cấp dùng Timer1 interrupt** (nếu cần chính xác hơn):
```c
void ultrasonic_trigger(void) {
    output_high(TRIG_PIN);
    delay_us(10);
    output_low(TRIG_PIN);
    
    // Setup Timer1 capture mode
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_1);
    setup_ccp3(CCP_CAPTURE_RE);  // Capture rising edge
}

#INT_CCP3
void ccp3_isr(void) {
    static uint16_t rise_time = 0;
    
    if(input(ECHO_PIN)) {
        // Rising edge - start measurement
        rise_time = get_timer1();
    } else {
        // Falling edge - calculate pulse width
        uint16_t fall_time = get_timer1();
        uint16_t pulse_width = fall_time - rise_time;
        // Process pulse_width...
    }
}
```

### 5. State Machine Best Practices

**Entry/Update/Exit pattern**:
- **Entry**: Khởi tạo, set flags, start timers
- **Update**: Logic chính, check điều kiện chuyển state
- **Exit**: Dọn dẹp, reset flags, stop timers

**Tránh deadlock**:
```c
// ❌ WRONG - có thể bị kẹt
if(condition1 && condition2 && condition3) {
    transition(NEXT_STATE);
}

// ✅ CORRECT - có timeout protection
if(condition1 && condition2 && condition3) {
    transition(NEXT_STATE);
} else if((ms_tick - state_entry_time) > TIMEOUT) {
    transition(STATE_ERROR);
}
```

### 6. Motor Direction Logic

**TB6612 Truth Table**:
```
AIN1 | AIN2 | Motor A
-----|------|--------
  1  |  0   | Forward
  0  |  0   | Brake (Fast stop)
  1  |  1   | Brake
  0  |  1   | Reverse

STBY = 0 → Standby (motor off)
STBY = 1 → Active
```

**Tại sao AIN2/BIN2 = GND**:
- Đơn giản hóa logic (chỉ cần 1 pin direction)
- Forward: AIN1=1, AIN2=0
- Reverse: AIN1=0, AIN2=0
- Không có brake mode (không cần thiết)

### 7. Lost Line Recovery Strategy

**3 cấp độ xử lý**:

1. **Level 1 - Maintain direction** (0-500ms):
```c
if(sensor_pattern == PATTERN_LOST) {
    if(lost_line_time == 0) lost_line_time = ms_tick;
    if((ms_tick - lost_line_time) < 500) {
        motor_apply_table(last_valid_pattern);
        return;  // Tiếp tục hướng cũ
    }
}
```

2. **Level 2 - Backup and search** (error_count < 3):
```c
motor_reverse(BACK_PWM);
delay_ms(300);
motor_stop();
// Kiểm tra lại sensor
```

3. **Level 3 - Manual intervention** (error_count >= 3):
```c
motor_stop();
// Chờ STOP button → manual reposition → RUN
```

### 8. Checkpoint System Implementation

**Sử dụng EEPROM** (optional):
```c
#include <internal_eeprom.c>

void checkpoint_save(uint8_t checkpoint_id) {
    write_eeprom(0, checkpoint_id);
    write_eeprom(1, station_pass_count);
}

uint8_t checkpoint_load(void) {
    checkpoint_state = read_eeprom(0);
    station_pass_count = read_eeprom(1);
    return checkpoint_state;
}
```

**Hoặc dùng biến volatile** (hiện tại):
- Reset khi tắt nguồn
- Đơn giản hơn
- Không cần EEPROM write cycles

### 9. Timing Calibration

**Đo thời gian backup 250mm**:
```c
// Test code:
void calibrate_backup_distance(void) {
    motor_reverse(BACK_PWM);
    
    // Đo bằng thước, điều chỉnh BACK_DELAY_MS
    for(uint16_t t = 0; t < 1000; t += 50) {
        delay_ms(50);
        printf("Time: %ums\n", t);
        // Đo khoảng cách thực tế tại mỗi bước
    }
    
    motor_stop();
}
```

**Công thức ước lượng**:
```
distance = speed * time
250mm = BACK_PWM * k * time
→ time = 250 / (BACK_PWM * k)

Với BACK_PWM = 120/255 ≈ 47%
Giả sử max_speed = 0.5 m/s
→ actual_speed = 0.5 * 0.47 = 0.235 m/s
→ time = 0.25 / 0.235 ≈ 1.06s ≈ 1000ms

Nhưng thực tế cần test và điều chỉnh!
```

### 10. Interrupt Priority

**PIC18 có 2 mức priority**:
```c
// High priority (thời gian thực):
#INT_TIMER0 HIGH
void timer0_isr(void) { ... }

// Low priority:
#INT_RB LOW
void iocb_isr(void) { ... }
```

**Cấu hình priority**:
```c
void interrupt_init(void) {
    enable_interrupts(INT_TIMER0);
    enable_interrupts(INT_RB);
    
    #ifdef USE_PRIORITY
    enable_interrupts(GLOBAL_HIGH);
    enable_interrupts(GLOBAL_LOW);
    #else
    enable_interrupts(GLOBAL);
    #endif
}
```

### 11. RAM Optimization

**PIC18F2685**: 3328 bytes RAM

**Ước lượng sử dụng**:
- Global variables: ~50 bytes
- FSM state variables: ~20 bytes
- Stack: ~100 bytes
- Chatter filter: 3 bytes
- **Total**: ~200 bytes

**Tối ưu hóa nếu cần**:
```c
// Dùng bit fields:
typedef struct {
    unsigned run_flag : 1;
    unsigned stop_flag : 1;
    unsigned ball_ok : 1;
    unsigned turn_complete : 1;
    // Tiết kiệm 75% so với 4 bytes bool
} Flags_t;

// Dùng uint8_t thay uint16_t nếu có thể
// Dùng local variables thay global
// Dùng const ROM cho lookup tables
```

### 12. Power Management

**Tùy chọn tiết kiệm pin**:
```c
void power_save_mode(void) {
    // Giảm clock
    setup_oscillator(OSC_4MHZ);
    
    // Tắt ADC
    setup_adc(ADC_OFF);
    
    // Sleep khi idle
    if(current_state == STATE_IDLE) {
        sleep();  // Wake on interrupt
    }
}
```

### 13. Debug UART (Optional)

**Thêm UART debug**:
```c
// Trong config.h:
#use rs232(UART1, baud=9600, parity=N, bits=8)

// Debug macro:
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) 
#endif

// Sử dụng:
DEBUG_PRINT("State: %u, Pattern: %03b\n", current_state, sensor_pattern);
```

### 14. Testing Checklist

**Hardware tests**:
- [ ] IR sensors: Đọc 0/1 đúng trên đen/trắng
- [ ] Motor: Quay đúng chiều A forward, B forward
- [ ] PWM: Oscilloscope đo 20kHz
- [ ] Ultrasonic: Đọc khoảng cách chính xác
- [ ] LEDs: Sáng/tắt đúng
- [ ] Buttons: Debounce hoạt động

**Software tests**:
- [ ] Chatter filter: Loại bỏ nhiễu < 3 samples
- [ ] Lookup table: Pattern → Motor command đúng
- [ ] State transitions: Entry/Exit gọi đúng
- [ ] Timing: Timer0 2ms chính xác
- [ ] Lost line: Maintain direction 500ms
- [ ] Error recovery: Retry 3 lần

**Integration tests**:
- [ ] Bám line thẳng
- [ ] Rẽ cua trái/phải
- [ ] Vượt ngã 4
- [ ] Dừng tại station
- [ ] Lấy bi
- [ ] Lùi và rẽ
- [ ] Dừng tại END
- [ ] Thả bi

### 15. Common Issues & Solutions

**Issue 1: Motor không quay**
- Check: STBY pin = HIGH?
- Check: PWM output có tín hiệu?
- Check: Direction pins đúng logic?
- Check: Nguồn motor đủ?

**Issue 2: Sensor nhiễu**
- Tăng CHATTER_N lên 5
- Thêm capacitor filter phần cứng
- Giảm tốc độ đọc sensor

**Issue 3: Xe lạc hướng**
- Calibrate sensor threshold
- Điều chỉnh lookup table speeds
- Kiểm tra surface contrast

**Issue 4: Ultrasonic không ổn định**
- Thêm retry logic (đọc 3 lần, lấy median)
- Tăng timeout
- Check mounting angle

**Issue 5: Xe không dừng đúng điểm**
- Fine-tune STOP_CM thresholds
- Thêm overshooting compensation
- Dùng PID cho smooth stopping

---

**Good luck! 🚀**