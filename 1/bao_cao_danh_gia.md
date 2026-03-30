# BÁO CÁO ĐÁNH GIÁ CODEBASE - ROBOT DÒ LINE PIC18F2685

## PHẦN I: TỔNG QUAN ĐÁNH GIÁ

### 1. Mức độ hoàn thiện: **85/100**

Codebase đã đáp ứng **hầu hết yêu cầu cơ bản** để điều khiển xe dò line với các tính năng:
- ✅ Máy trạng thái FSM 9 states hoàn chỉnh
- ✅ Lookup table cho 8 patterns cảm biến
- ✅ Điều khiển motor TB6612FNG với PWM 20kHz
- ✅ Đo khoảng cách HC-SR04
- ✅ Chatter filter chống nhiễu cảm biến
- ✅ Checkpoint EEPROM
- ⚠️ Một số vấn đề quan trọng cần sửa

---

## PHẦN II: CÁC VẤN ĐỀ NGHIÊM TRỌNG CẦN SỬA NGAY

### 🔴 VẤN ĐỀ 1: XÉT COLLISION PIN MAP (CỰC KỲ QUAN TRỌNG)

**File `config.h`:**
```c
#define MOTOR_PWMB       PIN_C3    // PWM for Motor B (Right)
#define SERVO_GRAB       PIN_C4    // Servo for ball grabber
```

**File Excel `overview_7.xlsx` - Sheet PCB:**
```
RC3 → LED D2 (LED2)
RC4 → Dự phòng (để trống)
```

**⚠️ XUNG ĐỘT PIN NGHIÊM TRỌNG:**
1. **Code định nghĩa RC3 = MOTOR_PWMB** (PWM motor phải)
2. **Excel PCB định nghĩa RC3 = LED D2** (LED Action)
3. **Code định nghĩa RC4 = SERVO_GRAB**, nhưng **Excel để RC4 trống**

**💡 GIẢI PHÁP:**

**Phương án A: Sửa theo Excel (KHUYẾN NGHỊ)**
```c
// config.h - SỬA LẠI
#define LED_ACTION       PIN_C3    // LED Action (theo PCB)
#define SERVO_GRAB       PIN_C4    // Servo gắp bi (dùng pin dự phòng)
#define RELAY_RELEASE    PIN_C5    // Relay thả bi

// motor.c - GIỮ NGUYÊN
// CCP1 → RC2 (Motor trái)
// Không dùng CCP2, dùng software PWM cho motor phải
```

**Phương án B: Sửa lại PCB (tốn thời gian hơn)**
- Chuyển LED2 sang RC4
- Giữ RC3 cho CCP2/PWMB

---

### 🔴 VẤN ĐỀ 2: SOFTWARE PWM CHO MOTOR PHẢI KHÔNG ĐÚNG

**File `motor.c` dòng 18-22:**
```c
volatile int8 duty_R = 0;
volatile int8 timer2_count = 0;

#INT_TIMER2
void timer2_isr(void) {
    if (timer2_count < duty_R) {
        output_high(MOTOR_PWMB);  // RC3 HIGH
    } else {
        output_low(MOTOR_PWMB);
    }
    timer2_count++;
}
```

**❌ LỖI LOGIC:**
1. Timer2 đã dùng cho CCP1 PWM (motor trái)
2. Không thể dùng Timer2 ISR đồng thời cho software PWM
3. `timer2_count++` sẽ overflow sau 256 lần nhưng không reset

**💡 GIẢI PHÁP: DÙNG TIMER3 CHO SOFTWARE PWM**

```c
// motor.c - SỬA LẠI
volatile int8 duty_R = 0;
volatile int8 timer3_count = 0;

#INT_TIMER3
void timer3_isr(void) {
    // Timer3 ngắt mỗi 50µs (20kHz)
    if (timer3_count < duty_R) {
        output_high(PIN_C1);  // Hoặc PIN nào đó cho motor phải
    } else {
        output_low(PIN_C1);
    }
    
    timer3_count++;
    if (timer3_count >= 255) {
        timer3_count = 0;  // Reset counter
    }
}

void motor_init(void) {
    // Setup Timer2 cho CCP1 (motor trái)
    setup_timer_2(T2_DIV_BY_1, 249, 1);
    setup_ccp1(CCP_PWM);
    
    // Setup Timer3 cho software PWM (motor phải)
    // Timer3 @ 5MHz, muốn 20kHz → period = 50µs = 250 ticks
    setup_timer_3(T3_INTERNAL | T3_DIV_BY_1);
    set_timer3(65536 - 250);  // Reload value
    enable_interrupts(INT_TIMER3);
}
```

---

### 🟡 VẤN ĐỀ 3: LOGIC XỬ LÝ MẤT LINE CHƯA TỐI ƯU

**File `fsm.c` - STATE_FOLLOW_LINE:**
```c
if (sensors_is_lost(pattern)) {
    if (line_lost_time == 0) {
        line_lost_time = ms_tick;
    }
    
    if ((ms_tick - line_lost_time) > LINE_LOST_MS) {
        fsm_handle_error(ERR_LOST_LINE);
        return;
    }
    
    cmd = motor_get_command(sensors_get_last_valid());  // Duy trì hướng cũ
    motor_set(cmd.left_pwm, cmd.right_pwm, cmd.direction);
    return;
}

line_lost_time = 0;  // Reset khi tìm lại line
```

**⚠️ VẤN ĐỀ:**
- Không có counter đếm số lần liên tiếp mất line
- `LINE_LOST_MS = 300ms` có thể quá ngắn với tốc độ cao

**💡 CẢI TIẾN:**
```c
static int8 line_lost_counter = 0;
#define LINE_LOST_COUNT_MAX 5  // 5 lần liên tiếp @ 10ms/loop = 50ms

if (sensors_is_lost(pattern)) {
    line_lost_counter++;
    
    if (line_lost_counter >= LINE_LOST_COUNT_MAX) {
        // Thử giảm tốc trước khi báo lỗi
        if (line_lost_counter < LINE_LOST_COUNT_MAX + 10) {
            MotorCmd_t cmd = motor_get_command(sensors_get_last_valid());
            motor_set(cmd.left_pwm / 2, cmd.right_pwm / 2, cmd.direction);
        } else {
            fsm_handle_error(ERR_LOST_LINE);
        }
        return;
    }
    
    // Duy trì hướng cũ với tốc độ giảm dần
    MotorCmd_t cmd = motor_get_command(sensors_get_last_valid());
    int8 speed_factor = (LINE_LOST_COUNT_MAX - line_lost_counter) * 20;
    motor_set(cmd.left_pwm - speed_factor, 
              cmd.right_pwm - speed_factor, 
              cmd.direction);
    return;
}

line_lost_counter = 0;  // Reset khi tìm lại line
```

---

### 🟡 VẤN ĐỀ 4: CHATTER FILTER CHƯA ĐƯỢC GỌI ĐÚNG

**File `sensors.c`:**
```c
void sensors_chatter_filter(int8 new_reading) {
    chatter_buffer[chatter_index] = new_reading;
    chatter_index = (chatter_index + 1) % CHATTER_N;
    
    int1 all_same = TRUE;
    for (int8 i = 1; i < CHATTER_N; i++) {
        if (chatter_buffer[i] != chatter_buffer[0]) {
            all_same = FALSE;
            break;
        }
    }
    
    if (all_same) {
        filtered_pattern = chatter_buffer[0];
        if (sensors_is_valid_line(filtered_pattern)) {
            last_valid_pattern = filtered_pattern;
        }
    }
}
```

**File `main.c` - Timer0 ISR:**
```c
#INT_TIMER0
void timer0_isr(void) {
    set_timer0(63036);
    ms_tick += 2;
    
    int8 raw_pattern = sensors_read_raw();
    sensors_chatter_filter(raw_pattern);  // ✅ ĐÚNG
    
    led_tick++;
    if (led_tick >= 125) {
        led_tick = 0;
    }
}
```

**✅ PHẦN NÀY ĐÃ ĐÚNG** - Chatter filter được gọi trong ISR mỗi 2ms.

---

### 🟡 VẤN ĐỀ 5: ULTRASONIC TIMEOUT CHƯA ĐƯỢC XỬ LÝ TỐT

**File `ultrasonic.c`:**
```c
int16 ultrasonic_measure_pulse(void) {
    int32 timeout_counter = 0;
    
    // Chờ echo lên HIGH
    while (!input(ULTRA_ECHO)) {
        delay_us(1);
        timeout_counter++;
        if (timeout_counter > ULTRA_TIMEOUT) {
            return 0;  // Timeout
        }
    }
    
    // Reset Timer1
    set_timer1(0);
    
    // Chờ echo xuống LOW
    timeout_counter = 0;
    while (input(ULTRA_ECHO)) {
        delay_us(1);
        timeout_counter++;
        if (timeout_counter > ULTRA_TIMEOUT) {
            return 0;  // Timeout
        }
    }
    
    int16 ticks = get_timer1();
    int16 pulse_width = ticks / 5;
    
    return pulse_width;
}
```

**⚠️ VẤN ĐỀ:**
- `ULTRA_TIMEOUT = 30000` loops × 1µs = 30ms blocking time
- Blocking quá lâu, ảnh hưởng đến bám line

**💡 CẢI TIẾN: DÙNG TIMER1 OVERFLOW THAY VÌ COUNTER**

```c
int16 ultrasonic_measure_pulse(void) {
    set_timer1(0);
    int16 timeout_ticks = 0;
    
    // Chờ echo lên HIGH với timeout 5ms
    while (!input(ULTRA_ECHO)) {
        if (get_timer1() > 25000) {  // 5ms @ 5MHz = 25000 ticks
            return 0xFFFF;  // Timeout
        }
    }
    
    // Reset và đo xung
    set_timer1(0);
    
    // Chờ echo xuống LOW với timeout 25ms
    while (input(ULTRA_ECHO)) {
        timeout_ticks = get_timer1();
        if (timeout_ticks > 125000) {  // 25ms = 4.3m
            return 0xFFFF;  // Quá xa
        }
    }
    
    int16 pulse_width = timeout_ticks / 5;  // Convert to µs
    return pulse_width;
}

int8 ultrasonic_read_cm(void) {
    int16 pulse_us = ultrasonic_measure_pulse();
    
    if (pulse_us == 0xFFFF) {
        return ULTRA_ERROR;
    }
    
    // Total timeout: 5ms (wait) + 25ms (measure) = 30ms max
    int16 distance_cm = pulse_us / 58;
    
    return (distance_cm > 254) ? 254 : (int8)distance_cm;
}
```

---

## PHẦN III: CÁC VẤN ĐỀ CẦN CẢI TIẾN (KHÔNG NGHIÊM TRỌNG)

### 🟢 VẤN ĐỀ 6: THIẾU DEBOUNCE CHO NÚT NHẤN

**File `main.c` - Port B ISR:**
```c
#INT_RB
void portb_change_isr(void) {
    int8 portb = input_b();
    
    if (!input(BUTTON_RUN)) {
        delay_ms(50);  // ✅ CÓ debounce rồi
        if (!input(BUTTON_RUN)) {
            run_flag = TRUE;
        }
    }
    
    if (input(BALL_SENSOR)) {
        ball_grabbed = TRUE;  // ❌ THIẾU debounce
    }
}
```

**💡 CẢI TIẾN:**
```c
#INT_RB
void portb_change_isr(void) {
    int8 portb = input_b();
    
    // Button debounce
    if (!input(BUTTON_RUN)) {
        delay_ms(50);
        if (!input(BUTTON_RUN)) {
            run_flag = TRUE;
        }
    }
    
    // Ball sensor debounce
    if (input(BALL_SENSOR)) {
        delay_ms(20);  // Thêm debounce
        if (input(BALL_SENSOR)) {
            ball_grabbed = TRUE;
        }
    }
}
```

---

### 🟢 VẤN ĐỀ 7: MOTOR PIVOT FUNCTIONS CHƯA CHUẨN

**File `motor.c`:**
```c
void motor_pivot_right(int8 speed) {
    set_pwm1_duty((int16)speed * 4);
    duty_R = speed;
    output_low(MOTOR_AIN1);    // Left forward
    output_high(MOTOR_BIN1);   // Right reverse
}
```

**⚠️ COMMENT SAI:**
- Comment nói "Left forward, Right reverse"
- Nhưng code: `output_low(MOTOR_AIN1)` = REVERSE (nếu AIN2=GND)

**💡 SỬA COMMENT:**
```c
void motor_pivot_right(int8 speed) {
    // Pivot right: Left reverse, Right forward
    set_pwm1_duty((int16)speed * 4);
    duty_R = speed;
    output_low(MOTOR_AIN1);    // Left reverse (AIN1=0, AIN2=0 via TB6612)
    output_high(MOTOR_BIN1);   // Right forward (BIN1=1, BIN2=0)
}

void motor_pivot_left(int8 speed) {
    // Pivot left: Left forward, Right reverse
    set_pwm1_duty((int16)speed * 4);
    duty_R = speed;
    output_high(MOTOR_AIN1);   // Left forward (AIN1=1, AIN2=0)
    output_low(MOTOR_BIN1);    // Right reverse (BIN1=0, BIN2=0)
}
```

**LƯU Ý QUAN TRỌNG:**
Kiểm tra kỹ logic TB6612FNG:
- Nếu AIN2 = GND cố định:
  - AIN1=1 → Forward
  - AIN1=0 → Brake to GND
- Cần xác nhận lại với datasheet TB6612FNG

---

### 🟢 VẤN ĐỀ 8: STATE_NAVIGATION CHỜ LINE CHƯA TỐI ƯU

**File `fsm.c`:**
```c
void state_navigation_update(void) {
    int8 pattern = sensors_read_filtered();
    int32 turn_duration = ms_tick - nav_start_time;
    
    if (turn_duration < NAV_MIN_MS) {
        return;  // Chờ tối thiểu 200ms
    }
    
    if ((pattern & 0x02) != 0) {  // SM thấy line
        fsm_transition(STATE_FOLLOW_LINE);
    }
    
    if (turn_duration > 2000) {
        fsm_handle_error(ERR_LOST_LINE);
    }
}
```

**💡 CẢI TIẾN: THÊM LOGIC XÁC NHẬN LINE**

```c
void state_navigation_update(void) {
    int8 pattern = sensors_read_filtered();
    int32 turn_duration = ms_tick - nav_start_time;
    
    if (turn_duration < NAV_MIN_MS) {
        return;
    }
    
    // Xác nhận line: SM thấy + (SL hoặc SR thấy)
    int1 sm_on = (pattern & 0x02) != 0;
    int1 side_on = (pattern & 0x05) != 0;  // SL or SR
    
    if (sm_on && side_on && turn_duration > NAV_MIN_MS) {
        fsm_transition(STATE_FOLLOW_LINE);
        return;
    }
    
    // Timeout dài hơn cho phép rẽ khúc lớn
    if (turn_duration > 3000) {
        fsm_handle_error(ERR_LOST_LINE);
    }
}
```

---

### 🟢 VẤN ĐỀ 9: EEPROM CHECKPOINT CHƯA ĐƯỢC SỬ DỤNG ĐẦY ĐỦ

**File `fsm.c`:**
```c
void fsm_save_checkpoint(Checkpoint_t cp) {
    current_checkpoint = cp;
    write_eeprom(EEPROM_CHECKPOINT, cp);
    write_eeprom(EEPROM_STATE, current_state);
}
```

**💡 BỔ SUNG: LƯU THÊM THÔNG TIN**

```c
#define EEPROM_CHECKPOINT    0x00
#define EEPROM_STATE         0x01
#define EEPROM_CUR_NODE      0x02  // Lưu vị trí node hiện tại
#define EEPROM_BALL_STATUS   0x03  // Đã lấy bi chưa

void fsm_save_checkpoint(Checkpoint_t cp) {
    current_checkpoint = cp;
    write_eeprom(EEPROM_CHECKPOINT, cp);
    write_eeprom(EEPROM_STATE, current_state);
    write_eeprom(EEPROM_CUR_NODE, cur_node);
    write_eeprom(EEPROM_BALL_STATUS, ball_grabbed ? 1 : 0);
}

void state_checkpoint_update(void) {
    current_checkpoint = read_eeprom(EEPROM_CHECKPOINT);
    int8 saved_node = read_eeprom(EEPROM_CUR_NODE);
    int8 saved_ball = read_eeprom(EEPROM_BALL_STATUS);
    
    // Khôi phục trạng thái
    cur_node = saved_node;
    ball_grabbed = (saved_ball == 1);
    
    switch (current_checkpoint) {
        case CP_START:
            fsm_transition(STATE_FOLLOW_LINE);
            break;
            
        case CP_AFTER_STATION:
            sensors_led_action(TRUE);
            fsm_transition(STATE_NAVIGATION);
            break;
            
        case CP_BEFORE_END:
            sensors_led_action(TRUE);
            fsm_transition(STATE_FOLLOW_LINE);
            break;
            
        default:
            fsm_transition(STATE_FOLLOW_LINE);
            break;
    }
}
```

---

### 🟢 VẤN ĐỀ 10: LOOKUP TABLE TỐC ĐỘ CHƯA ĐƯỢC ĐIỀU CHỈNH

**File `motor.c`:**
```c
const rom MotorCmd_t motor_table[8] = {
    {BASE_PWM, BASE_PWM, DIR_FORWARD},      // 000: Line lost
    {BASE_PWM, SLOW_PWM, DIR_FORWARD},      // 001: Strong right
    {BASE_PWM, BASE_PWM, DIR_FORWARD},      // 010: Centered
    {BASE_PWM, BASE_PWM - 30, DIR_FORWARD}, // 011: Light right
    {SLOW_PWM, BASE_PWM, DIR_FORWARD},      // 100: Strong left
    {STOP_PWM, STOP_PWM, DIR_FORWARD},      // 101: T-junction
    {BASE_PWM - 30, BASE_PWM, DIR_FORWARD}, // 110: Light left
    {BASE_PWM, BASE_PWM, DIR_FORWARD}       // 111: Intersection
};
```

**💡 ĐIỀU CHỈNH TỐI ƯU HƠN:**

```c
// Thêm constants
#define TURN_LIGHT_REDUCTION 40   // Giảm 25% cho lệch nhẹ
#define TURN_SHARP_REDUCTION 80   // Giảm 50% cho lệch mạnh

const rom MotorCmd_t motor_table[8] = {
    // 000: Line lost - giữ nguyên hướng
    {BASE_PWM, BASE_PWM, DIR_FORWARD},
    
    // 001: Strong right - xe lệch trái nhiều, rẽ phải gắt
    {BASE_PWM, SLOW_PWM, DIR_FORWARD},
    
    // 010: Centered - đi thẳng
    {BASE_PWM, BASE_PWM, DIR_FORWARD},
    
    // 011: Light right - xe lệch trái nhẹ, rẽ phải nhẹ
    {BASE_PWM, BASE_PWM - TURN_LIGHT_REDUCTION, DIR_FORWARD},
    
    // 100: Strong left - xe lệch phải nhiều, rẽ trái gắt
    {SLOW_PWM, BASE_PWM, DIR_FORWARD},
    
    // 101: T-junction - dừng để FSM xử lý
    {STOP_PWM, STOP_PWM, DIR_FORWARD},
    
    // 110: Light left - xe lệch phải nhẹ, rẽ trái nhẹ
    {BASE_PWM - TURN_LIGHT_REDUCTION, BASE_PWM, DIR_FORWARD},
    
    // 111: Intersection - đi thẳng qua
    {BASE_PWM, BASE_PWM, DIR_FORWARD}
};
```

---

## PHẦN IV: CÁC ĐIỂM MẠNH CỦA CODEBASE

### ✅ 1. KI TRÚC FSM RÕ RÀNG
- 9 states được phân tách rõ ràng
- Entry/Update/Exit pattern chuẩn
- Transition function tập trung

### ✅ 2. CHATTER FILTER ĐƯỢC TRIỂN KHAI ĐÚNG
- N=3 samples liên tiếp
- Gọi trong ISR mỗi 2ms
- Lọc nhiễu hiệu quả

### ✅ 3. LOOKUP TABLE TIẾT KIỆM RAM
- Sử dụng `const ROM` đặt trong FLASH
- Truy cập O(1) nhanh
- Dễ điều chỉnh

### ✅ 4. CHECKPOINT EEPROM
- Lưu trạng thái vào non-volatile memory
- Cho phép recovery sau lỗi

### ✅ 5. CẤU TRÚC FILE RÕ RÀNG
- Tách module rõ ràng: sensors, motor, ultrasonic, fsm
- Header files đầy đủ
- Comments chi tiết

---

## PHẦN V: DANH SÁCH CÁC FILE CẦN SỬA

### 🔴 PRIORITY 1 (NGUY CẤP - PHẢI SỬA NGAY)

1. **config.h**
   - Sửa pin map collision RC3
   - Đồng bộ với PCB Excel
   
2. **motor.c**
   - Chuyển software PWM sang Timer3
   - Sửa logic pivot functions
   
3. **motor.h**
   - Cập nhật comments

### 🟡 PRIORITY 2 (QUAN TRỌNG - NÊN SỬA)

4. **fsm.c**
   - Cải tiến logic xử lý mất line
   - Thêm xác nhận line trong navigation
   - Bổ sung checkpoint data

5. **ultrasonic.c**
   - Tối ưu timeout mechanism
   - Giảm blocking time

### 🟢 PRIORITY 3 (TỐT NẾU CÓ)

6. **main.c**
   - Thêm debounce cho ball sensor

7. **sensors.c**
   - Có thể bổ sung calibration tự động

---

## PHẦN VI: KẾ HOẠCH SỬA CHỮA

### BƯỚC 1: SỬA PIN MAP (1-2 giờ)
1. Xác định PIN layout cuối cùng với PCB
2. Sửa `config.h`
3. Test lại tất cả peripheral

### BƯỚC 2: SỬA MOTOR CONTROL (2-3 giờ)
1. Implement Timer3 ISR cho software PWM
2. Test motor độc lập
3. Verify pivot functions

### BƯỚC 3: CẢI TIẾN FSM LOGIC (2-3 giờ)
1. Cải tiến line lost handling
2. Test với track thật
3. Tune thông số

### BƯỚC 4: TỐI ƯU ULTRASONIC (1 giờ)
1. Sửa timeout mechanism
2. Test đo khoảng cách

### BƯỚC 5: FULL SYSTEM TEST (3-4 giờ)
1. Test từng module
2. Test tích hợp
3. Test trên track thật
4. Fine-tune parameters

**TỔNG THỜI GIAN ƯỚC TÍNH: 9-13 giờ**

---

## PHẦN VII: KẾT LUẬN

### ĐÃ HOÀN THÀNH TỐT:
✅ Kiến trúc FSM chuẩn
✅ Lookup table tối ưu
✅ Chatter filter đúng chuẩn
✅ Cấu trúc code rõ ràng
✅ Documentation đầy đủ

### CẦN SỬA NGAY:
🔴 Pin map collision (RC3)
🔴 Software PWM Timer2 conflict
🔴 Motor pivot logic unclear

### NÊN CẢI TIẾN:
🟡 Line lost handling
🟡 Navigation line detection
🟡 Ultrasonic timeout
🟡 Checkpoint data richness

### ĐÁNH GIÁ CUỐI CÙNG:
**Code CÓ THỂ CHẠY ĐƯỢC** sau khi sửa các lỗi PRIORITY 1.
Với các cải tiến PRIORITY 2 và 3, chất lượng sẽ đạt **professional level**.

---

## PHỤ LỤC: TEST CHECKLIST

### A. UNIT TEST (Test từng module)
- [ ] GPIO init - Kiểm tra ADCON1 = 0x0F
- [ ] Sensors read - Test 8 patterns
- [ ] Chatter filter - Test với noise
- [ ] Motor PWM - Test cả 2 motor độc lập
- [ ] Ultrasonic - Test từ 2cm đến 200cm
- [ ] EEPROM - Test read/write checkpoint

### B. INTEGRATION TEST (Test tích hợp)
- [ ] FSM transitions - Test tất cả 19 cung chuyển
- [ ] Sensor → Motor - Test lookup table
- [ ] Ultrasonic → Stop - Test dừng chính xác
- [ ] Button → State change - Test interrupt
- [ ] LED indicators - Test tất cả states

### C. SYSTEM TEST (Test trên track)
- [ ] Bám line thẳng
- [ ] Bám line cong nhẹ/gắt
- [ ] Vượt ngã 3
- [ ] Vượt ngã 4
- [ ] Dừng tại Station
- [ ] Lấy bi
- [ ] Lùi sau lấy bi
- [ ] Quay tại chỗ
- [ ] Đến END
- [ ] Thả bi
- [ ] Error recovery
- [ ] Checkpoint recovery

---

**Người đánh giá:** Claude (AI Assistant)
**Ngày:** 30/03/2026
**Version:** 1.0
