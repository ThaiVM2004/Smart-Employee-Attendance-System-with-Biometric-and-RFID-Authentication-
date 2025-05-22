#include <Arduino.h>
#include <HardwareSerial.h>

// UART giao tiếp với ESP32-CAM (UART2)
HardwareSerial SerialCam(2);

// UART giao tiếp với STM32 (UART1)
HardwareSerial STM32_Serial(1);

// Gán chân UART
#define RXD_CAM 16
#define TXD_CAM 17

#define RXD_STM32 12
#define TXD_STM32 14

void setup() {
  Serial.begin(115200);  // Debug qua USB
  delay(1000);

  // Giao tiếp với STM32 (RX=12, TX=14)
  STM32_Serial.begin(9600, SERIAL_8N1, RXD_STM32, TXD_STM32);
  delay(500);

  // Giao tiếp với ESP32-CAM (RX=16, TX=17)
  SerialCam.begin(115200, SERIAL_8N1, RXD_CAM, TXD_CAM);
  delay(500);

  Serial.println("ESP32-Master sẵn sàng.");
}

void loop() {
  // Xử lý nhập lệnh từ Serial
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    // Kiểm tra nếu là lệnh "D" hoặc "d"
    if(input.equalsIgnoreCase("D")) {
      STM32_Serial.println(input);
      Serial.print("Đã gửi lệnh đến STM32: "); 
      Serial.println(input);
    } 
    // Kiểm tra nếu là ID (chỉ chứa số)
    else if (input.length() > 0 && isDigit(input[0])) {
      STM32_Serial.println(input);
      Serial.print("Đã gửi ID đến STM32: "); 
      Serial.println(input);
    }
    // Các lệnh khác gửi đến ESP32-CAM
    else if (input.length() > 0) {
      SerialCam.println(input);
      Serial.print("Đã gửi đến ESP32-CAM: "); 
      Serial.println(input);
    }
  }

  // 2. Nhận dữ liệu từ STM32 và gửi cho ESP32-CAM
  if (STM32_Serial.available()) {
    String uid = STM32_Serial.readStringUntil('\n');
    uid.trim();
    if (uid.length() > 0) {
      SerialCam.println(uid);
      Serial.println("Đã gửi UID từ STM32 tới ESP32-CAM: " + uid);
    }
  }

  // 3. Nhận phản hồi từ ESP32-CAM và xử lý có chọn lọc
  if (SerialCam.available()) {
    String resp = SerialCam.readStringUntil('\n');
    resp.trim();

    // Điều kiện lọc: phản hồi phải là "OK" hoặc có dấu phẩy để phân biệt ID,Name
    if (resp == "OK" || resp.indexOf(',') > 0) {
      Serial.println("ESP32-CAM phản hồi hợp lệ: " + resp);
      STM32_Serial.println(resp);  // Gửi về STM32
    } else {
      // Bỏ qua các dòng phản hồi HTML hoặc log rác
    }
  }

  delay(50);
}