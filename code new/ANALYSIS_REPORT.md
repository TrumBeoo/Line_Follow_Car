# Báo Cáo Phân Tích Code - Line Follower Robot
**Ngày**: 02/04/2026  
**Mục tiêu**: Đảm bảo 3 Timer hoạt động ổn định, không có xung đột giữa các tiến trình

---

## 📊 TÓM TẮT VẤN ĐỀ

| Mức độ | Số lượng | Chi tiết |
|-------|---------|---------|
| **🔴 QUAN TRỌNG** | 3 | Race condition, Timer không đồng bộ, Button debounce |
| **🟠 TRUNG BÌNH** | 6 | Blocking, EEPROM, Sensor filter, FSM safety |
| **✅ Total** | **9 vấn đề** | Yêu cầu sửa chữa |

---

## 🔴 VẤN ĐỀ QUAN TRỌNG

### 1️⃣ RACE CONDITION - Volatile Variables (Nguy Hiểm)

**📍 Vị trí**: [main.c](main.c#L41-L50), [main.c](main.c#L1055-L1064)

**Vấn đề**: 
- Biến volatile (system_enabled, run_flag, stop_pressed, ms_tick, led_tick) được **đọc từ main loop** mà **KHÔNG disable interrupt**
- Chỉ `get_ms_tick()` (dòng 1027) có bảo vệ - disable GLOBAL interrupt thời gian đọc
- Các volatile khác (led_tick, system_enabled) được đọc TRỰC TIẾP trong main loop
  
**Code lỗi** - dòng 1055-1062:
```c
while (TRUE) {
    if (!system_enabled) {  // ❌ Đọc volatile KHÔNG có disable interrupt
        delay_ms(10);
        continue;
    }
    handle_checkpoint_buttons();  // ❌ Dùng cp1_long_press - volatile
    // ...
}
```

**Hệ quả**:
- **Torn read**: ISR thay đổi system_enabled trong khi main loop đang đọc → giá trị bất thường
- **State machine nhảy bất thường**: run_flag, stop_pressed bị sai lệch
- **Dữ liệu không nhất quán** → Robot không hoạt động đúng

**✅ Giải pháp**: Disable GLOBAL interrupt khi đọc tất cả volatile trong main loop

---

### 2️⃣ TIMER0 & TIMER2 KHÔNG ĐỒNG BỘ (Bất Ổn)

**📍 Vị trí**: [main.c](main.c#L862-L872), [main.c](main.c#L221-L228)

**Vấn đề**:
- **Timer0** chạy ở ~1ms (prescale=8, reload=63036 ticks)
  ```c
  setup_timer_0(RTCC_INTERNAL | RTCC_DIV_8);
  set_timer0(63036);  // 1ms period
  ```

- **Timer2** chạy ở ~2ms (prescale=4, period=49)
  ```c
  setup_timer_2(T2_DIV_BY_4, 49, 1);  // 2ms @ 20MHz
  ```

- Chúng **hoạt động độc lập** → **KHÔNG đồng bộ**

**Hiệu ứng xấu**:
- 🔄 **Phase drift**: Timer0 ISR bắn ở ms 0, 1, 2, 3... nhưng Timer2 ISR bắn ở ms 0, 2, 4...
- Một lúc chúng chạy gần nhau, lúc khác cách xa → **PWM 不穩定**
- **Motor có thể bị nhấp nháy** vì tốc độ PWM không nhất quán

**Ví dụ kém may**:
```
ms:  0    1    2    3    4    5    6
T0: [INT] [INT] [INT] [INT] [INT] [INT]    ← Mỗi 1ms
T2:      [INT]      [INT]       [INT]      ← Không đồng bộ, bất thường
```

**✅ Giải pháp**: Sync Timer0 & Timer2 bằng cách:
- Chạy cả hai từ cùng prescaler
- Hoặc dùng một ISR chính để trigger các hành động 1ms, 2ms khác nhau

---

### 3️⃣ BUTTON DEBOUNCE - ISR KHÔNG CÓ DEBOUNCE (Một)

**📍 Vị trị**: [main.c](main.c#L938-L973)

**Vấn đề**:
```c
#INT_RB
void portb_change_isr(void) {
    int8 portb;
    int32 press_duration;
    
    portb = input_b();  // ❌ Đọc trực tiếp - không debounce
    
    if (!input(BUTTON_STOP)) {  // ❌ Lại đọc trực tiếp
        stop_press_time = get_ms_tick();
        stop_pressed = TRUE;
    } else if (stop_pressed) {
        // Tính thời gian nhấn
        press_duration = get_ms_tick() - stop_press_time;
        // ...
    }
}
```

**Vấn đề cụ thể**:
- **Tiếp xúc cơ học** gây bounce: nút bấm > nút nhả > nút bấm lại (3-5 lần trong 10-50ms)
- ISR được gọi **mỗi lần PORT thay đổi** → có thể 3-5 lần cho 1 nhấn nút
- run_flag, stop_pressed được set nhiều lần → FSM nhân bản hành động
- Long-press detection sai vì stop_press_time reset nhiều lần

**Hệ quả**:
- 🔴 **Nút STOP kích hoạt 3-5 lần** thay vì 1 lần
- **Checkpoint save sai**, FSM transition bất thường
- **RUN button** có thể trigger multiple action

**✅ Giải pháp**: Thêm software debounce
- Lưu lần cuối ISR được gọi (dùng ms_tick)
- Bỏ qua ISR nếu < 10ms kể từ lần trước
- Hoặc dùng counter thay vì boolean flag

---

## 🟠 VẤN ĐỀ TRUNG BÌNH (Quan trọng nhưng không tức thời)

### 4️⃣ MAIN LOOP BUSY-WAIT + ULTRASONIC BLOCKING

**📍 Vị trí**: [main.c](main.c#L463), [main.c](main.c#L377-L403)

**Vấn đề**:
```c
// Main loop gọi state update (dòng 1055):
state_follow_line_update();  // ← Gọi liên tục

// state_follow_line_update() (dòng 456-497):
void state_follow_line_update(void) {
    int8 pattern;
    int8 distance;
    
    pattern  = sensors_read_filtered();      // ← Từ ISR, OK
    distance = ultrasonic_read_cm();         // ❌ BLOCKING!
    // ...
}

// ultrasonic_read_cm() (dòng 409-418):
int8 ultrasonic_read_cm(void) {
    ultrasonic_trigger();                    // ← Trigger sensor
    delay_us(50);                            // Chờ
    pulse_us = ultrasonic_measure_pulse();  // ← BLOCKING loop!
    // ...
}

// ultrasonic_measure_pulse() (dòng 377-403):
int16 ultrasonic_measure_pulse(void) {
    // ...
    while (!input(ULTRA_ECHO)) {
        delay_us(1);                         // ❌ Loop busy-wait
        if (++timeout_counter > 30000) return 0;
    }
    // ...
}
```

**Hệ quả**:
- Main loop có thể **block 1-2ms** chỉ để đo khoảng cách
- Nếu object xa → chờ đầy ~30ms!
- Trong khi chờ: **ISR vẫn chạy** (Timer0, Timer2, PORTB) → nhưng main loop không responsive
- Nếu ISR set flag, main loop chậm respond

**⚠️ Vấn đề thứ 2**: Sensor được đọc từ ISR (chatter_filter ở Timer0 - dòng 878) mỗi 1ms
- Nhưng main loop đọc sensor ở tốc độ tùy ý (có thể 10ms/lần)
- Mất tính nhất quán, có thể miss sensor event

**✅ Giải pháp**:
- Cache kết quả ultrasonic vào ISR (chạy định kỳ, không block)
- Main loop chỉ đọc cache
- Hoặc dùng state machine cho ultrasonic: trigger → chờ → read

---

### 5️⃣ FSM TRANSITION KHÔNG INTERRUPT-SAFE

**📍 Vị trí**: [main.c](main.c#L790-L845), [main.c](main.c#L960-L973)

**Vấn đề**:
```c
void fsm_transition(SystemState_t next_state) {
    // exit handler cho state cũ
    switch (current_state) { /* ... */ }
    
    previous_state = current_state;
    current_state  = next_state;
    state_entry_time = get_ms_tick();  // ❌ Dùng shared variable!
    
    // entry handler cho state mới
    switch (current_state) { /* ... */ }
}
```

**Vấn đề**:
- Gọi từ 2 chỗ:
  1. **Main loop** (dòng 1057-1061) - khi state_update() call fsm_transition()
  2. **PORTB ISR** (dòng 960) - khi nút bấm gọi fsm_transition()

- Nếu PORTB ISR gọi fsm_transition() khi main loop **đang trong fsm_transition()** → **NESTED CALL**
  - state_entry_time được set lại (dòng 794)
  - Entry handler được gọi lại ngay
  - State machine bị lộn xộn

**Case xấu**:
```c
// Thời điểm T1: Main loop
state_follow_line_update() {
    // ...
    fsm_transition(STATE_NAVIGATION);  // ← Bắt đầu transition
        state_follow_line_exit();      // ← Đang chạy
        // **INTERRUPT: PORTB ISR được gọi!**
        // PORTB ISR:
        fsm_transition(STATE_IDLE);    // ← Enter FSM lại!
        // → state_entry_time thay đổi
        // → STATE_NAVIGATION entry không được gọi!
    // ...
}
```

**✅ Giải pháp**: Disable interrupt trong fsm_transition() hoặc dùng flag để ngăn ISR gọi fsm_transition()

---

### 6️⃣ SENSOR CHATTER FILTER - Response Slow

**📍 Vị trí**: [main.h](main.h#L110), [main.c](main.c#L878)

**Vấn đề**:
- Chatter filter cần **3 mẫu giống nhau** (CHATTER_N=3) trước khi accept
- Gọi từ Timer0 ISR **mỗi 1ms** → phải chờ ~3ms để filter respond

```c
#define CHATTER_N 3

void sensors_chatter_filter(int8 new_reading) {
    chatter_buffer[chatter_index] = new_reading;
    chatter_index = (chatter_index + 1) % CHATTER_N;
    
    // Check consensus
    all_same = TRUE;
    for (i = 1; i < CHATTER_N; i++) {
        if (chatter_buffer[i] != chatter_buffer[0]) {
            all_same = FALSE;
            break;
        }
    }
    
    if (all_same) {  // ← Mất 3ms để đạt đây
        filtered_pattern = chatter_buffer[0];
    }
}
```

**Hệ quả**:
- Nếu robot di chuyển nhanh qua junction, có thể **bỏ lỡ junction pattern** (0b111 hoặc 0b101)
- Độ trễ 3ms có thể không chấp nhận được ở tốc độ cao

**✅ Giải pháp**: Thay CHATTER_N = 2 (2ms debounce) hoặc dùng majority voting

---

### 7️⃣ EEPROM WRITE KHÔNG TIMEOUT

**📍 Vị trí**: [main.c](main.c#L802-L806)

**Vấn đề**:
```c
void fsm_save_checkpoint(Checkpoint_t cp) {
    current_checkpoint = cp;
    write_eeprom(0, (int8)cp);       // ❌ Không timeout
    write_eeprom(1, (int8)nav_direction);
    write_eeprom(2, intersection_count);
}
```

- EEPROM write thường mất **2-5ms**
- Trong khi write, nếu ISR cần access EEPROM → conflict
- Nếu write bị interrupt, dữ liệu có thể corruption

**✅ Giải pháp**: 
- Dùng ISR-safe EEPROM routine (CCS compiler có cung cấp)
- Hoặc disable interrupt trong fsm_save_checkpoint()

---

### 8️⃣ LED BLINK COUNTER - Shared Variable

**📍 Vị trí**: [main.c](main.c#L888-L902)

**Vấn đề**:
- led_tick dùng chung trong Timer0 ISR
- Main loop có thể check `led_tick % 200` (dòng 898) khi ISR increment nó
- Torn read có thể xảy ra

**✅ Giải pháp**: Cache LED state trong Timer0 ISR, main loop chỉ đọc cache

---

### 9️⃣ ISR CHẠY DÀI - Sensor Read Trong ISR

**📍 Vị trí**: [main.c](main.c#L862-L902)

**Vấn đề**:
- Timer0 ISR gọi `sensors_chatter_filter()` (dòng 878)
- Hàm này loop chạy chậm (dòng 121-128)
- Nếu ISR mất > 1ms → overlap Timer0 interrupts → lỗi!

---

## 📋 BẢNG TÓNG KẾT FIX

| # | Vấn đề | Mức độ | Fix | Độ khó | Ưu tiên |
|---|--------|-------|-----|--------|---------|
| 1 | Race condition volatile | 🔴 | Disable INT khi đọc | Dễ | 🥇 1 |
| 2 | Timer không sync | 🔴 | Unified timer ISR | TB | 🥇 2 |
| 3 | Button debounce | 🔴 | SW debounce 10ms | Dễ | 🥇 3 |
| 4 | Ultrasonic blocking | 🟠 | Cache + ISR read | Trung | 🥈 4 |
| 5 | FSM transition safety | 🟠 | Disable INT in fsm | TB | 🥈 5 |
| 6 | Chatter filter slow | 🟠 | Thay CHATTER_N=2 | Dễ | 🥈 6 |
| 7 | EEPROM timeout | 🟠 | Safe write | TB | 🥈 7 |
| 8 | LED counter shared | 🟠 | Cache | Dễ | 🥉 8 |
| 9 | ISR thời gian | 🟠 | Profile | Dễ | 🥉 9 |

---

## 🛠️ TIẾN HÀNH SỬA CHỮA

Tất cả các fix đã được chuẩn bị trong: **main_FIXED.c**

Ưu tiên:
1. **Hôm nay**: Fix #1, #2, #3 (3 vấn đề QUAN TRỌNG)
2. **Tuần này**: Fix #4, #5, #6, #7 (Trung bình)
3. **Sau**: Fix #8, #9 (Tối ưu)

