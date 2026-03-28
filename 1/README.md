# LINE FOLLOWER ROBOT - PIC18F2685

## 📋 TỔNG QUAN DỰ ÁN

Dự án robot dò line với khả năng:
- Bám đường line đen trên nền trắng
- Nhận diện ngã 3, ngã 4, điểm dừng
- Lấy bi tại Station
- Thả bi tại điểm END
- Hệ thống Checkpoint để khôi phục sau lỗi

## 🔧 PHẦN CỨNG

### Vi điều khiển
- **MCU**: PIC18F2685
- **Clock**: 20MHz (Crystal HS mode)
- **Memory**: 48KB Flash, 3.3KB RAM, 1KB EEPROM

### Cảm biến
- **IR Line Sensors**: 3 cảm biến (Left, Mid, Right) - Active LOW
- **Ultrasonic**: HC-SR04 (Trigger: RB4, Echo: RB5)
- **Ball Sensor**: Digital sensor tại RB3 (optional)

### Động cơ
- **Driver**: TB6612FNG Dual Motor Driver
- **PWM**: CCP1 (RC2), CCP2 (RC3) @ 20kHz
- **Control**: AIN1 (RB0), BIN1 (RB2), STBY (RB6)

### Ngoại vi
- **LED Status**: RC0 (hoạt động)
- **LED Action**: RC1 (lấy/thả bi)
- **Servo Grab**: RC4 (gắp bi)
- **Relay Release**: RC5 (thả bi)
- **Button RUN**: RB7 (với interrupt)

## 📁 CẤU TRÚC THƯ MỤC

```
Project_Robot_DoLine/
├── config.h          # Cấu hình phần cứng, fuses, pin mapping
├── main.h            # Header tổng quát
├── main.c            # Chương trình chính và ISR
├── fsm.h             # Định nghĩa máy trạng thái
├── fsm.c             # Logic 9 trạng thái
├── motor.h           # Định nghĩa điều khiển motor
├── motor.c           # PWM và lookup table
├── sensors.h         # Định nghĩa cảm biến IR
├── sensors.c         # Chatter filter và đọc sensor
├── ultrasonic.h      # Định nghĩa siêu âm
├── ultrasonic.c      # Đo khoảng cách HC-SR04
└── README.md         # File này
```

## 🎯 LOGIC ĐIỀU KHIỂN

### 1. Đi thẳng (Pattern: 010)
- Hai motor cùng tốc độ BASE_PWM
- Tự động điều chỉnh nếu lệch

### 2. Mất line (Pattern: 000)
- Duy trì hướng cũ trong 300ms
- Nếu quá lâu → chuyển sang ERROR state

### 3. Ngã 3 (Pattern: 101)
- Phát hiện Station gần (< 3cm)
- Dừng, lấy bi 2s
- Lùi 250mm
- Quay phải tại chỗ

### 4. Ngã 4 (Pattern: 111)
- Đi thẳng qua 150ms
- Tiếp tục bám line

### 5. Khúc cua
- Pattern 011/110: lệch nhẹ
- Pattern 001/100: lệch mạnh → giảm tốc motor một bên

### 6. Station (Trạm lấy bi)
- Phát hiện: khoảng cách ≤ 2cm
- Dừng, kích hoạt servo grab
- Chờ 2s hoặc ball_sensor = 1

### 7. END (Điểm thả bi)
- Phát hiện: khoảng cách ≤ 5cm
- Tiến thêm 300ms để tiếp xúc
- Kích hoạt relay thả bi 2s
- Reset hệ thống

### 8. Checkpoint
- 3 checkpoint: START, AFTER_STATION, BEFORE_END
- Lưu vào EEPROM
- Nhấn STOP → đặt lại xe → nhấn RUN để tiếp tục

## 🔄 MÁY TRẠNG THÁI (FSM)

```
STATE_IDLE          → Chờ nút RUN
STATE_FOLLOW_LINE   → Bám line bình thường
STATE_STATION       → Tiếp cận Station (giảm tốc)
STATE_STATION_STOP  → Dừng tại Station, gắp bi
STATE_STATION_BACK  → Lùi 250mm
STATE_NAVIGATION    → Quay tại chỗ (pivot turn)
STATE_END           → Đến END, thả bi
STATE_ERROR         → Xử lý lỗi, chờ manual reset
STATE_CHECKPOINT    → Khôi phục từ checkpoint
```

## ⚙️ THÔNG SỐ KỸ THUẬT

### PWM Motor
- Tần số: 20kHz
- Duty cycle: 0-255 (8-bit)
- BASE_PWM: 160 (tốc độ bình thường)
- SLOW_PWM: 80 (tốc độ chậm)

### Chatter Filter
- N = 3 mẫu liên tiếp
- Sampling: mỗi 2ms (từ Timer0 ISR)
- Loại bỏ nhiễu < 6ms

### Khoảng cách
- Station stop: ≤ 2cm
- END stop: ≤ 5cm
- Reverse: 250mm (sau khi lấy bi)

### Thời gian
- Grab ball: 2000ms
- Release ball: 2000ms
- Line lost timeout: 300ms
- Navigation min: 200ms
- Cross intersection: 150ms

## 🚀 BIÊN DỊCH VÀ NẠP

### Với CCS C Compiler

1. **Mở dự án trong CCS IDE**
   - File → Open → Chọn main.c
   - Hoặc tạo New Project với PIC18F2685

2. **Cấu hình Compiler**
   - Device: PIC18F2685
   - Clock: 20MHz
   - Include Directories: thư mục chứa các file .h

3. **Build**
   - Project → Build All
   - Kiểm tra không có lỗi

4. **Nạp vào MCU**
   - Sử dụng PICkit 3/4 hoặc ICD
   - File .hex được tạo trong thư mục output

### Với MPLAB X + XC8

```bash
# Tạo project mới
# Device: PIC18F2685
# Compiler: XC8
# Add tất cả file .c và .h vào project
# Build project
```

## 📊 LOOKUP TABLE (Motor Commands)

| Pattern | Binary | Mô tả              | Left PWM | Right PWM | Direction |
|---------|--------|--------------------|----------|-----------|-----------|
| 0       | 000    | Line lost          | 160      | 160       | Forward   |
| 1       | 001    | Strong right       | 160      | 80        | Forward   |
| 2       | 010    | Centered           | 160      | 160       | Forward   |
| 3       | 011    | Light right        | 160      | 130       | Forward   |
| 4       | 100    | Strong left        | 80       | 160       | Forward   |
| 5       | 101    | T-junction/Station | 0        | 0         | Forward   |
| 6       | 110    | Light left         | 130      | 160       | Forward   |
| 7       | 111    | Intersection       | 160      | 160       | Forward   |

## 🐛 TROUBLESHOOTING

### Robot không khởi động
- Kiểm tra nguồn 5V
- Kiểm tra LED nháy khi bật nguồn
- Kiểm tra fuses (HS, NOWDT, NOLVP, MCLR)

### Không bám line
- Kiểm tra cảm biến IR (đảo bit chưa?)
- Kiểm tra chatter filter
- Tăng/giảm BASE_PWM nếu quá nhanh/chậm

### Không dừng tại Station
- Kiểm tra kết nối HC-SR04
- Test hàm ultrasonic_read_cm()
- Điều chỉnh STOP_CM_STATION

### Không quay được
- Kiểm tra motor_pivot_right/left
- Kiểm tra logic điều khiển AIN1, BIN1
- Kiểm tra STBY pin = HIGH

## 📞 HỖ TRỢ

Nếu gặp vấn đề:
1. Kiểm tra kết nối phần cứng
2. Test từng module riêng (sensors, motor, ultrasonic)
3. Sử dụng LED để debug trạng thái
4. Kiểm tra EEPROM checkpoint values

## 📝 GHI CHÚ

- **QUAN TRỌNG**: gpio_init() phải được gọi ĐẦU TIÊN trong system_init()
- Tất cả biến được sửa trong ISR phải là `volatile`
- Sử dụng `const ROM` cho lookup table để tiết kiệm RAM
- Timer0 ISR mỗi 2ms → độ phân giải thời gian 2ms

## 📄 LICENSE

Educational/Academic Project - Free to use and modify

## ✨ FEATURES NÂNG CAO (Tương lai)

- [ ] PID control cho bám line mượt hơn
- [ ] Adaptive speed dựa trên độ cong đường
- [ ] Machine learning cho sensor calibration
- [ ] Wireless debugging qua UART
- [ ] LCD hiển thị trạng thái
- [ ] Multiple route support

---

**Version**: 1.0
**Date**: March 2026
**Author**: Robot Line Follower Team
