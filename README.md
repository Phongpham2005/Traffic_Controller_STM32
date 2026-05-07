# Real-time Traffic Light Controller (STM32F103)

Hệ thống điều khiển đèn giao thông tại ngã tư sử dụng vi điều khiển **STM32F103C8T6**. Dự án tập trung vào việc tối ưu hóa hiệu suất thông qua kiến trúc hướng sự kiện (Event-driven) và quản lý thời gian bằng ngắt phần cứng (Hardware Interrupt).

## 🚀 Tính năng nổi bật

- **Kiến trúc Time-Triggered:** Sử dụng Hardware Timer ngắt mỗi 1ms để tạo "nhịp đập" cho hệ thống, đảm bảo tính chính xác tuyệt đối về thời gian.
- **Xử lý đa nhiệm phi đồng bộ:** 
    - Quét nút bấm và chống dội phím (Debouncing) ngầm.
    - Quét LED 7 đoạn (Multiplexing) ở tần số cao (200Hz) không gây chớp giật.
- **Hệ thống FSM (Finite State Machine):** Quản lý các trạng thái đèn giao thông thông minh.
- **Chế độ Setting nâng cao:** 
    - Cho phép thay đổi thời gian các đèn Xanh/Vàng/Đỏ.
    - Tự động tính toán ràng buộc thời gian: `Đỏ = Xanh + Vàng`.
    - Giới hạn biên dưới và xử lý tràn số hiển thị (hiện 99 khi bộ đếm > 99).


## 🛠 Quy trình phát triển (Development Workflow)

Dự án được triển khai theo quy trình chuẩn của một kỹ sư nhúng (Embedded Workflow), từ thiết kế phần cứng, cấu hình ngoại vi đến thực thi phần mềm:

### 1. Cấu hình ngoại vi với STM32CubeMX
Sử dụng công cụ CubeMX để cấu hình tài nguyên phần cứng một cách tối ưu:
- **Xung nhịp (Clock):** Thiết lập hệ thống chạy ở tần số tối đa **72MHz** bằng thạch anh ngoài (HSE) để đảm bảo độ chính xác cho các bộ đếm thời gian.
- **Timer 2:** Cấu hình bộ chia (Prescaler) và giá trị nạp (Period) để tạo ngắt định kỳ chính xác mỗi **1ms** (1000Hz). Đây là "nhịp tim" điều phối toàn bộ hệ thống.
- **GPIO:** Thiết lập nhãn (Label) cho các chân điều khiển LED, nút bấm và LED 7 đoạn để tăng tính minh bạch và dễ bảo trì cho mã nguồn.

### 2. Phát triển mã nguồn trên KeilC V5
Triển khai logic điều khiển dựa trên bộ thư viện chuẩn của ST (HAL Driver):
- **Tách biệt Logic (Decoupling):** Sử dụng các hàm Callback ngắt (`HAL_TIM_PeriodElapsedCallback`) để tách biệt giữa việc thu thập dữ liệu (Input) và xử lý logic (Main Loop).
- **Quản lý trạng thái (State Management):** Sử dụng `struct` để quản lý các nút bấm và `enum` để quản lý 5 chế độ hoạt động của đèn giao thông, giúp code gãy gọn và dễ mở rộng.
- **Tối ưu hóa hiển thị:** Thuật toán quét LED ma trận được tính toán kỹ lưỡng để không gây hiện tượng bóng ma (Ghosting) và đảm bảo độ sáng đồng đều.

### 3. Mô phỏng và Kiểm thử với Proteus 8 Professional
Kiểm tra tính đúng đắn của firmware trước khi triển khai thực tế:
- **Sơ đồ nguyên lý:** Thiết kế mạch điện tử với các linh kiện tương đương hệ thống thực (STM32, Nút nhấn, LED 7 đoạn Anode chung).
- **Kiểm thử logic:** Xác minh các ràng buộc thời gian đặc biệt như `Red = Green + Yellow` và khả năng lưu trữ giá trị khi thay đổi tham số trong `Setting Mode`.
- **Hiệu năng:** Đảm bảo hệ thống phản hồi tức thì với thao tác nút bấm của người dùng và không có độ trễ trong quá trình chuyển đổi trạng thái đèn.

## 📈 Kết quả đạt được
- Hệ thống hoạt động ổn định 24/7 theo kiến trúc Time-Triggered.
- Logic đếm ngược hiển thị số `0` đúng 1 giây trước khi chuyển trạng thái (khắc phục hoàn toàn lỗi mất nhịp thường gặp).
- Khả năng chống nhiễu phần mềm cực tốt cho hệ thống 8 nút điều khiển.

## 🛠 Linh kiện & Công cụ sử dụng

- **Vi điều khiển:** STM32F103C8T6 (Blue Pill).
- **Hiển thị:** 2 cụm LED 7 đoạn đôi (7SEG-MPX2-CA).
- **Môi trường lập trình:** KeilC V5, STM32CubeMX.
- **Thư viện:** STM32 HAL Driver.
- **Phần mềm mô phỏng:** Proteus 8 Professional.

## 🕹 Các chế độ hoạt động (Modes)

1.  **Auto Mode:** Chạy tự động theo chu kỳ thời gian đã cài đặt.
2.  **Night Mode:** Đèn vàng nhấp nháy 500ms, LED 7 đoạn hiển thị `00`.
3.  **Manual NS GO:** Ưu tiên hướng Bắc-Nam xanh liên tục, LED 7 đoạn hiện `99`.
4.  **Manual EW GO:** Ưu tiên hướng Đông-Tây xanh liên tục, LED 7 đoạn hiện `99`.
5.  **Setting Mode:** 
    - Nút **Select**: Chọn loại đèn cần chỉnh (Xanh -> Vàng -> Đỏ). Đèn đang chọn sẽ nhấp nháy.
    - Nút **Up/Down**: Tăng giảm thời gian.
    - Nút **Set**: Lưu cấu hình và quay lại chế độ Tự động.
