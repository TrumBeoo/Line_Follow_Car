# LINE FOLLOWER ROBOT - PIC18F2685

Dự án xe dò line sử dụng vi điều khiển PIC18F2685, driver motor TB6612, 3 cảm biến IR và cảm biến siêu âm HC-SR04.

## 🎯 Tính năng

- **Bám line thông minh** với Lookup Table (không dùng PID)
- **Chatter Filter** 3 mẫu loại bỏ nhiễu sensor
- **Xử lý ngã 3, ngã 4** tự động
- **Lấy bi tại Station** bằng cảm biến siêu âm
- **Thả bi tại END** point
- **3 Checkpoint** phục hồi sau lỗi
- **Error Recovery** tự động với retry logic

## 📁 Cấu trúc thư mục

```
Project_Robot_DoLine/
├── main.c              # Luồng chính, vòng lặp FSM, ISR
├── main.h              # Header tổng quát
├── config.h            # Cấu hình fuses và pin map
├── fsm.c               # Logic 10 trạng thái máy
├── fsm.h               # Khai báo FSM
├── motor.c             # Lookup table và điều khiển PWM
├── motor.h             # Khai báo motor
├── sensors.c           # Chatter filter và đọc IR
├── sensors.h           # Khai báo sensors
├── ultrasonic.c        # Đo khoảng cách HC-SR04
├── ultrasonic.h        # Khai báo ultrasonic
└── README.md           # File này
```

## 🔌 Sơ đồ kết nối

### Cảm biến IR (Active-LOW)
- **RA0** - Sensor Left (SL)
- **RA1** - Sensor Middle (SM)
- **RA2** - Sensor Right (SR)

### Driver motor TB6612
- **RC2** - PWMA (CCP1)
- **RC3** - PWMB (CCP2)
- **RB0** - AIN1 (Motor A direction)
- **RB2** - BIN1 (Motor B direction)
- **RB6** - STBY (Standby control)
- **AIN2, BIN2** - Nối GND (cố định)

### Siêu âm HC-SR04
- **RB4** - TRIG (Trigger)
- **RB5** - ECHO (Echo)

### Cơ cấu bi
- **RC4** - Servo/Gắp bi
- **RC5** - Cơ cấu thả bi

### LED & Nút bấm
- **RC0** - LED Status
- **RC1** - LED Action
- **RB7** - Nút RUN
- **RB3** - Nút STOP / Cảm biến bi

## ⚙️ Thông số kỹ thuật

- **MCU**: PIC18F2685
- **Clock**: 20MHz (Crystal HS)
- **PWM**: 20kHz (8-bit: 0-255)
- **Timer0**: 2ms system tick
- **Base Speed**: 160/255 (~63%)
- **Turn Speed**: 120/255 (~47%)
- **Back Speed**: 120/255 (~47%)

## 🚀 Hướng dẫn biên dịch

### Với CCS C Compiler

1. Mở CCS C IDE
2. Create New Project:
   - Device: PIC18F2685
   - Add all `.c` and `.h` files
3. Set include paths để compiler tìm được các header files
4. Build Project (Ctrl+F9)
5. Flash vào chip qua PICKIT3/4 hoặc ICD3

### Lưu ý biên dịch

- Đảm bảo file `18F2685.h` có trong thư viện CCS
- Kiểm tra cấu hình Fuses trong `config.h` phù hợp với hardware
- Clock 20MHz phải khớp với crystal thực tế

### Cấu hình Fuses quan trọng

```c
#fuses HS              // High Speed Crystal 20MHz
#fuses NOWDT           // Tắt Watchdog Timer
#fuses NOLVP           // Tắt Low Voltage Programming
#fuses NOPBADEN        // Port B Digital mode
```

## 🧠 Hoạt động của FSM

### 10 States

1. **IDLE** - Chờ nút RUN
2. **FOLLOW_LINE** - Bám line bình thường với lookup table
3. **STATION** - Tiếp cận trạm, giảm tốc
4. **STATION_STOP** - Dừng và gắp bi (2s)
5. **STATION_BACK** - Lùi 250mm
6. **NAVIGATION** - Xử lý ngã 3/ngã 4, rẽ phải
7. **END** - Thả bi tại điểm cuối (2s)
8. **ERROR** - Phục hồi lỗi, retry 3 lần
9. **CHECKPOINT_1** - Khởi động từ checkpoint 1
10. **CHECKPOINT_2** - Khởi động từ checkpoint 2

### Flow chính

```
IDLE → FOLLOW_LINE → STATION → STATION_STOP → STATION_BACK 
     → NAVIGATION → FOLLOW_LINE → END → IDLE
```

## 🎮 Sử dụng

### Khởi động bình thường
1. Đặt xe tại vị trí START
2. Nhấn nút **RUN**
3. Xe tự động bám line → lấy bi → thả bi

### Sử dụng Checkpoint
1. Khi xe gặp lỗi, nhấn **STOP**
2. Đặt xe tại checkpoint (1 hoặc 2)
3. Nhấn **RUN**, xe tự động tiếp tục từ checkpoint

### LED chỉ thị
- **LED Status (RC0)**:
  - Sáng: Đang hoạt động
  - Nhấp nháy: Lỗi
  - Tắt: IDLE
  
- **LED Action (RC1)**:
  - Sáng: Đang thao tác bi (gắp/thả)

## 🔧 Lookup Table

| Pattern | L | M | R | Ý nghĩa | Motor L | Motor R |
|---------|---|---|---|---------|---------|---------|
| 0b000 | 0 | 0 | 0 | Mất line | STOP | STOP |
| 0b001 | 0 | 0 | 1 | Lệch phải mạnh | BASE | TURN |
| 0b010 | 0 | 1 | 0 | **Chính giữa** | BASE | BASE |
| 0b011 | 0 | 1 | 1 | Lệch phải nhẹ | BASE | SLOW |
| 0b100 | 1 | 0 | 0 | Lệch trái mạnh | TURN | BASE |
| 0b101 | 1 | 0 | 1 | Ngã 3 | BASE | BASE |
| 0b110 | 1 | 1 | 0 | Lệch trái nhẹ | SLOW | BASE |
| 0b111 | 1 | 1 | 1 | **Ngã 4** | BASE | BASE |

## 🛡️ Xử lý lỗi

### Mất line (ERR_LOSTLINE)
1. Duy trì hướng cũ trong 500ms
2. Nếu vẫn mất → lùi lại tìm line
3. Retry tối đa 3 lần
4. Sau đó chờ can thiệp thủ công

### Timeout
- Navigation timeout: 2s
- Ultrasonic timeout: 255cm

## 📊 Thông số đo khoảng cách

### Siêu âm HC-SR04
- **Công thức**: `distance_cm = pulse_us / 58`
- **Dừng tại Station**: ≤ 2cm
- **Dừng tại END**: ≤ 5cm
- **Timeout**: 25ms (4m max range)

## ⏱️ Timing quan trọng

- **System Tick**: 2ms (Timer0)
- **Grab Ball**: 2000ms
- **Drop Ball**: 2000ms
- **Backup**: 250ms (~250mm)
- **Cross Intersection**: 150ms
- **Min Turn Duration**: 200ms
- **Lost Line Timeout**: 500ms
- **Button Debounce**: 50ms

## 🐛 Debug

### Kiểm tra hardware
1. Test từng sensor IR riêng lẻ
2. Kiểm tra TB6612 STBY pin = HIGH
3. Đo PWM trên RC2, RC3 (20kHz)
4. Test ultrasonic riêng

### Kiểm tra software
1. Thêm UART debug (optional)
2. Sử dụng LED để debug state
3. Kiểm tra Timer0 hoạt động (2ms)
4. Verify chatter filter (3 samples)

## 📝 Lưu ý quan trọng

1. **Thứ tự khởi tạo**: GPIO → Timer → PWM → Interrupt
2. **ADCON1 = 0x0F**: Bắt buộc cho digital I/O Port A
3. **Chatter Filter**: Cần 3 mẫu liên tiếp giống nhau
4. **Sensor Active-LOW**: Phải đảo bit (black=1 sau khi invert)
5. **TB6612 AIN2/BIN2**: Nối cố định GND
6. **STBY Pin**: Phải HIGH để motor hoạt động
7. **Volatile variables**: Cho biến được cập nhật trong ISR

## 🔄 EEPROM (Tương lai)

Có thể mở rộng để lưu:
- Quãng đường đã chạy
- Số lần lấy bi
- Cấu hình PID (nếu nâng cấp)
- Checkpoint positions

## 📚 Tài liệu tham khảo

- [PIC18F2685 Datasheet](https://www.microchip.com/PIC18F2685)
- [TB6612 Datasheet](https://www.sparkfun.com/TB6612)
- [HC-SR04 Datasheet](https://www.micropik.com/HC-SR04)
- [CCS C Compiler Manual](https://www.ccsinfo.com/)

## 👨‍💻 Tác giả

Dự án được phát triển cho môn học Embedded Systems

## 📄 License

MIT License - Free to use for educational purposes

---

**Chúc bạn thành công! 🚗💨**