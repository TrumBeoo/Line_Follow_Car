# Logic Xử Lý 2 Nút Nhấn Checkpoint - REFACTORED

## Nút 1 (BUTTON_STOP - PIN_B0) - Chức năng STOP và Cycle Checkpoint

### Nhấn giữ (>= 1 giây) - STOP Function:
- **Hành động**: Reset về trạng thái ban đầu
- **Checkpoint**: CP_START (= 1)  
- **Ball status**: ball_grabbed = FALSE (chưa có bi)
- **LED**: Tắt LED action
- **Motor**: Dừng motor
- **State**: Chuyển về STATE_IDLE
- **Mục đích**: Robot sẽ bám line và vào station lấy bi

### Nhấn thả (< 1 giây) - Cycle Through Checkpoints:
- **Từ CP_START**: Chuyển thành CP_START với ball_grabbed = TRUE
- **Từ CP_AFTER_NAVIGATION**: Chuyển thành CP_BEFORE_END
- **Từ CP_BEFORE_END**: Chuyển thành CP_AFTER_NAVIGATION
- **Lưu ý**: Chỉ thay đổi checkpoint, không tự động RUN

## Nút 2 (BUTTON_RUN_NEW - PIN_B1) - Chức năng RUN

### Nhấn thả - RUN from Current Checkpoint:
- **Từ CP_START**: 
  - Nếu ball_grabbed = FALSE: Bám line và vào station lấy bi
  - Nếu ball_grabbed = TRUE: Bỏ qua station, thẳng navigation
- **Từ CP_AFTER_NAVIGATION**: Đã có bi, tiếp tục bám line
- **Từ CP_BEFORE_END**: Gần END, đã có bi, tiếp tục đến END

## Checkpoint Definitions:
- **CP_START (1)**: Vị trí bắt đầu
- **CP_AFTER_NAVIGATION (2)**: Sau khi navigation thành công  
- **CP_BEFORE_END (3)**: Trước điểm END

## Workflow Sử Dụng:

1. **Đặt xe về vị trí checkpoint mong muốn**:
   - Nhấn thả nút STOP để cycle qua các checkpoint
   - LED action sáng = có bi, tắt = chưa có bi

2. **Chạy từ checkpoint đã chọn**:
   - Nhấn nút RUN để bắt đầu

3. **Reset hoàn toàn**:
   - Nhấn giữ nút STOP >= 1 giây

## Ứng Dụng Thực Tế:

- **Test từ đầu**: Nhấn giữ STOP → nhấn RUN
- **Test skip station**: Nhấn thả STOP (CP1 + bi) → nhấn RUN  
- **Test navigation**: Nhấn thả STOP 2 lần (CP2) → nhấn RUN
- **Test cuối đường**: Nhấn thả STOP 3 lần (CP3) → nhấn RUN