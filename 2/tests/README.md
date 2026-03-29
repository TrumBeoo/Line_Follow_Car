# 🧪 TEST MODULES - Line Follower Robot

## 📁 Cấu trúc thư mục tests/

```
tests/
├── test_common.h           # Header chung cho tất cả tests
├── test_motor.c            # Test motor control
├── test_sensors.c          # Test IR sensors + chatter filter
├── test_ultrasonic.c       # Test HC-SR04 ultrasonic
├── test_lookup_table.c     # Test lookup table 8 patterns
├── test_leds.c             # Test LED indicators
├── test_buttons.c          # Test RUN/STOP buttons
├── test_pwm.c              # Test PWM duty cycles
├── test_timer.c            # Test Timer0 accuracy
└── README.md               # File này
```

---

## 🎯 Mục đích

Mỗi file test một module riêng biệt để:
- ✅ **Debug dễ dàng**: Tìm lỗi nhanh trong module cụ thể
- ✅ **Độc lập**: Test không phụ thuộc lẫn nhau
- ✅ **Tái sử dụng**: Dùng lại cho project khác
- ✅ **Biên dịch nhanh**: Chỉ compile file cần test
- ✅ **Maintain tốt**: Thêm/sửa test không ảnh hưởng file khác

---

## 🚀 Cách sử dụng

### Bước 1: Chọn test cần chạy

Trong CCS C IDE:
1. File → New → Project
2. Chọn PIC18F2685
3. Add file test cần chạy (VD: `test_motor.c`)
4. Add các file module liên quan:
   - `motor.c`, `motor.h` (cho test_motor)
   - `sensors.c`, `sensors.h` (cho test_sensors)
   - `ultrasonic.c`, `ultrasonic.h` (cho test_ultrasonic)
   - v.v.

### Bước 2: Compile và flash

```bash
# Compile
Build Project (Ctrl+F9)

# Flash vào chip
Program → PIC18F2685
```

### Bước 3: Quan sát kết quả

Mỗi test có cách hiển thị kết quả riêng qua LED:

---

## 📋 Chi tiết từng test

### 1️⃣ test_motor.c - Test Motor Control

**Mục đích**: Kiểm tra tất cả chức năng motor

**Thời gian**: ~15 giây

**Trình tự**:
1. Forward BASE_PWM (2s)
2. Reverse BACK_PWM (2s)
3. Turn left (2s)
4. Turn right (2s)
5. Pivot left (1.5s)
6. Pivot right (1.5s)

**LED**:
- Status LED: Sáng khi test đang chạy
- Action LED: Nhấp nháy 0.5s khi hoàn thành

**Kiểm tra**:
- ✅ Motor quay đúng hướng
- ✅ Tốc độ ổn định
- ✅ TB6612 STBY hoạt động

---

### 2️⃣ test_sensors.c - Test IR Sensors

**Mục đích**: Kiểm tra đọc sensor và chatter filter

**Thời gian**: 60 giây

**LED**:
- Status LED: Sáng khi sensor ở CENTER (010)
- Action LED: Sáng khi phát hiện line (không phải 000)

**Kiểm tra**:
- ✅ Sensor đọc đúng black/white
- ✅ Chatter filter loại nhiễu
- ✅ Pattern chính xác

**Cách test**:
- Di chuyển line dưới sensor
- Quan sát LED thay đổi

---

### 3️⃣ test_ultrasonic.c - Test HC-SR04

**Mục đích**: Kiểm tra đo khoảng cách

**Thời gian**: 60 giây

**LED**:
- Cả 2 LED sáng: Khoảng cách ≤3cm
- Status LED sáng: Khoảng cách ≤5cm
- Action LED sáng: Khoảng cách ≤10cm
- Cả 2 tắt: Khoảng cách >10cm hoặc timeout

**Kiểm tra**:
- ✅ Đo khoảng cách chính xác
- ✅ Timeout khi không có vật cản
- ✅ Trigger/Echo hoạt động

**Cách test**:
- Đặt vật cản ở các khoảng cách khác nhau
- Quan sát LED thay đổi

---

### 4️⃣ test_lookup_table.c - Test Lookup Table

**Mục đích**: Kiểm tra 8 pattern trong lookup table

**Thời gian**: ~40 giây (8 pattern × 5s)

**Trình tự**: Test lần lượt pattern 000 → 111

**LED**:
- Status LED: Nhấp nháy số lần = số pattern (0-7)
- Action LED: Sáng khi motor đang chạy

**Kiểm tra**:
- ✅ Mỗi pattern cho tốc độ motor đúng
- ✅ Lookup table mapping chính xác

---

### 5️⃣ test_leds.c - Test LEDs

**Mục đích**: Kiểm tra tất cả chức năng LED

**Thời gian**: ~15 giây

**Trình tự**:
1. Alternating blink (10 lần)
2. Simultaneous blink (10 lần)
3. Fast toggle (20 lần)

**Kiểm tra**:
- ✅ LED sáng/tắt đúng
- ✅ Toggle hoạt động
- ✅ Timing chính xác

---

### 6️⃣ test_buttons.c - Test Buttons

**Mục đích**: Kiểm tra nút RUN và STOP với debounce

**Thời gian**: 60 giây

**LED**:
- Status LED: Toggle khi nhấn RUN (RB7)
- Action LED: Toggle khi nhấn STOP (RB3)

**Kiểm tra**:
- ✅ Nút nhấn phát hiện đúng
- ✅ Debounce 50ms hoạt động
- ✅ Không bị nhiễu

---

### 7️⃣ test_pwm.c - Test PWM

**Mục đích**: Kiểm tra PWM duty cycle trên CCP1/CCP2

**Thời gian**: ~25 giây

**Trình tự**: Test 6 mức duty: 0%, 25%, 50%, 63%, 75%, 100%

**LED**:
- Status LED: Nhấp nháy số lần = mức duty (0-5)
- Action LED: Sáng khi PWM đang chạy

**Kiểm tra**:
- ✅ PWM frequency = 20kHz
- ✅ Duty cycle chính xác
- ✅ CCP1/CCP2 hoạt động

**Dụng cụ**: Oscilloscope để đo PWM

---

### 8️⃣ test_timer.c - Test Timer0

**Mục đích**: Kiểm tra Timer0 2ms interrupt

**Thời gian**: 60 giây

**LED**:
- Status LED: Nhấp nháy mỗi 500ms (120 lần)
- Action LED: Toggle mỗi 10 lần

**Kiểm tra**:
- ✅ Timer0 chính xác 2ms
- ✅ ms_tick tăng đúng
- ✅ Interrupt hoạt động

**Kết quả**: Sau 60s, Status LED nhấp nháy ~12 lần = 120/10

---

## 🔧 Troubleshooting

### Lỗi compile

**Lỗi**: `Cannot find file test_common.h`

**Giải pháp**: 
- Add đường dẫn `tests/` vào include paths
- Hoặc copy `test_common.h` vào thư mục gốc

### Motor không chạy

**Kiểm tra**:
1. STBY pin = HIGH?
2. AIN1/BIN1 có tín hiệu?
3. PWM có xuất hiện trên RC2/RC3?
4. Nguồn motor đủ?

### Sensor không đọc

**Kiểm tra**:
1. ADCON1 = 0x0F? (Digital mode)
2. Sensor có nguồn?
3. Khoảng cách sensor-line phù hợp?

### Ultrasonic timeout

**Kiểm tra**:
1. Trigger pulse = 10µs?
2. Echo pin input mode?
3. Timer1 hoạt động?
4. Vật cản trong tầm (<4m)?

---

## 📊 So sánh với file cũ

| Tiêu chí | File cũ (1 file) | File mới (9 files) |
|----------|------------------|-------------------|
| Dễ debug | ❌ Khó | ✅ Dễ |
| Compile time | ❌ Chậm | ✅ Nhanh |
| Tái sử dụng | ❌ Khó | ✅ Dễ |
| Maintain | ❌ Khó | ✅ Dễ |
| Linh hoạt | ❌ Thấp | ✅ Cao |
| Số file | ✅ Ít (1) | ⚠️ Nhiều (9) |

---

## 🎯 Khuyến nghị

### Khi nào dùng file cũ (test_modules.c)?
- ✅ Test nhanh một lần
- ✅ Không cần debug chi tiết
- ✅ Ít thay đổi

### Khi nào dùng file mới (tests/)?
- ✅ **Development phase** (đang phát triển)
- ✅ **Debug từng module**
- ✅ **Maintain lâu dài**
- ✅ **Dự án lớn, nhiều người**

---

## 📝 Lưu ý

1. **Không chạy nhiều test cùng lúc**: Mỗi lần chỉ flash 1 file test
2. **Kiểm tra hardware trước**: Đảm bảo kết nối đúng
3. **Đọc kỹ LED indicator**: Mỗi test có cách hiển thị riêng
4. **Dùng oscilloscope**: Cho test PWM và Timer

---

## ✅ Checklist test đầy đủ

Trước khi chạy chương trình chính, test theo thứ tự:

- [ ] 1. test_leds.c - Kiểm tra LED hoạt động
- [ ] 2. test_timer.c - Kiểm tra Timer0 chính xác
- [ ] 3. test_buttons.c - Kiểm tra nút nhấn
- [ ] 4. test_pwm.c - Kiểm tra PWM 20kHz
- [ ] 5. test_motor.c - Kiểm tra motor control
- [ ] 6. test_sensors.c - Kiểm tra IR sensors
- [ ] 7. test_ultrasonic.c - Kiểm tra ultrasonic
- [ ] 8. test_lookup_table.c - Kiểm tra lookup table

**Nếu tất cả pass → Chạy main.c**

---

**Chúc bạn test thành công! 🚀**
