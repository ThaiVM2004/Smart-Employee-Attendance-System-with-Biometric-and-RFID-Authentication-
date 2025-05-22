#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Wire.h>
#include <MFRC522.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include <Adafruit_Fingerprint.h>

// Define pin connections
#define SDA_PIN PA4
#define RST_PIN PB0
#define LED_PIN PC13   // LED dùng để báo đã quét thành công

// System constants
#define FINGERPRINT_TIMEOUT 30000  // 30 giây chờ quét vân tay
#define MAX_FAIL_ATTEMPTS 3        // Số lần thất bại tối đa
#define NAME_DISPLAY_TIME 2000     // Thời gian hiển thị tên (2 giây)
#define ESP_RESPONSE_TIMEOUT 5000  // Thời gian chờ phản hồi từ ESP32
#define DISPLAY_MESSAGE_TIME 2000  // Thời gian hiển thị thông báo mặc định

// OLED
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire); // SCL: PB6, SDA: PB7

// Fingerprint sensor
HardwareSerial mySerial(USART2); //fingerprint 
HardwareSerial tranFor(USART1);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// RFID
MFRC522 mfrc522(SDA_PIN, RST_PIN);

// User database
String nameStr = "";
String idStr = "";
String receivedString = "";

// System state variables
unsigned long fingerprintTimeLimit = 0;
bool isCardDetected = false;
bool waitingForFingerprint = false;
uint8_t failCount = 0;
String currentMessage = "";

// Display text to OLED with better memory management
void drawString(const char *text, int textSize) {
  String newMessage = String(text);
  if (newMessage != currentMessage) {
    display.clearDisplay();
    int len = strlen(text);
    int textWidth = len * 6 * textSize;
    int x = (128 - textWidth) / 2;
    int y = (64 - 8 * textSize) / 2;
    display.setTextSize(textSize);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(x, y);
    display.print(text);
    display.display();
    currentMessage = newMessage;
  }
}

// Reset RFID reader để sẵn sàng đọc thẻ mới
void resetRFIDReader() {
  mfrc522.PCD_Reset();
  mfrc522.PCD_Init();
}

// Cải thiện hiệu suất xử lý chuỗi nhận từ ESP32
void processSerial() {
  while (tranFor.available()) {
    char c = tranFor.read();
    if (c == '\n') {
      // Tìm vị trí dấu phẩy để tách chuỗi
      int commaIndex = receivedString.indexOf(',');
      if (commaIndex > 0) {
        idStr = receivedString.substring(0, commaIndex);
        nameStr = receivedString.substring(commaIndex + 1);
        idStr.trim();
        nameStr.trim();
      }
      receivedString = ""; // Reset chuỗi sau khi xử lý
      return;
    } else {
      receivedString += c;
    }
  }
}

// Read RFID card với xử lý lỗi tốt hơn
void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;  // Không phát hiện thẻ mới
  }
  
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;  // Không đọc được thẻ
  }

  // Đọc RFID ID từ thẻ
  String uidHex = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidHex += (mfrc522.uid.uidByte[i] < 0x10 ? "0" : "") + String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) uidHex += ",";
  }
  uidHex.toUpperCase();

  // Giải phóng thẻ và ngừng giao tiếp crypto
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  // Gửi UID tới ESP32
  tranFor.println(uidHex);

  // LED báo hiệu
  digitalWrite(LED_PIN, LOW);
  delay(200);  // Giảm thời gian delay
  digitalWrite(LED_PIN, HIGH);

  // Chờ phản hồi từ ESP32
  unsigned long startWait = millis();
  idStr = ""; 
  nameStr = ""; 
  receivedString = "";
  
  while (nameStr.length() == 0 && millis() - startWait < 10000) {
    processSerial();
    // Thêm một khoảng thời gian nhỏ giữa các lần đọc để giảm tải CPU
    if (!tranFor.available()) {
      delay(5);
    }
  }

  if (nameStr.length() > 0) {
    drawString(nameStr.c_str(), 1);
    isCardDetected = true;
    waitingForFingerprint = true;
    
    // Hiển thị tên trong thời gian ngắn
    delay(NAME_DISPLAY_TIME);
    
    drawString("MOI QUET VAN TAY", 1);
    fingerprintTimeLimit = millis();
  } else {
    drawString("KHONG TIM THAY", 1);
    failCount++;
    delay(DISPLAY_MESSAGE_TIME);
    drawString("MOI QUET THE", 1);
    
    // Reset RFID reader để sẵn sàng cho lần quét tiếp theo
    resetRFIDReader();
  }
}

// Lấy ID vân tay
int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p == FINGERPRINT_NOFINGER) return -1;
  if (p != FINGERPRINT_OK) return 0;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return 0;

  p = finger.fingerSearch();
  if (p != FINGERPRINT_OK) return 0;

  return finger.fingerID;
}
//Đăng ký vân tay


// Xử lý vân tay hiệu quả hơn
void processFingerprint() {
  int fingerprintID = getFingerprintID();

  if (fingerprintID == -1) return; // Không có vân tay

  if (fingerprintID > 0) {
    // So sánh ID vân tay với idStr
    if (String(fingerprintID) == idStr) {
      drawString("DIEM DANH THANH CONG", 1);

      // Gửi tín hiệu đã quét thành công
      tranFor.println("e" + idStr);  

      // Chờ ESP32 phản hồi về việc chụp ảnh
      unsigned long startWait = millis();
      bool hasResponse = false;
      
      while (millis() - startWait < ESP_RESPONSE_TIMEOUT) {
        if (tranFor.available()) {
          String response = tranFor.readStringUntil('\n');
          response.trim();
          
          if (response == "OK") {
            drawString("DA CHUP ANH", 1);
            delay(DISPLAY_MESSAGE_TIME);
            drawString("DA GUI ANH THANH CONG", 1);
            hasResponse = true;
            break;
          } else {
            drawString("LOI CHUP ANH", 1);
            hasResponse = true;
            break;
          }
        }
        delay(10); // Giảm tải CPU trong vòng lặp chờ
      }
      
      delay(DISPLAY_MESSAGE_TIME);
      drawString("MOI QUET THE", 1);
      isCardDetected = false;
      waitingForFingerprint = false;
      failCount = 0; // Reset số lần thất bại
      
      // Reset RFID reader để sẵn sàng cho lần quét tiếp theo
      resetRFIDReader();
      return;
    } else {
      drawString("KHONG KHOP", 1);
      failCount++;
    }
  } else {
    drawString("KHONG HOP LE", 1);
    failCount++;
  }

  delay(DISPLAY_MESSAGE_TIME);
  
  // Kiểm tra số lần thất bại
  if (failCount >= MAX_FAIL_ATTEMPTS) {
    handleFailedAttempts();
  } else {
    drawString("MOI QUET VAN TAY", 1);
  }
}

// Xử lý khi có quá nhiều lần thất bại
void handleFailedAttempts() {
  drawString("CANH BAO!", 1);
  
  // Đếm ngược thời gian chờ
  for (int i = 15; i >= 1; i--) {
    char countText[20];
    sprintf(countText, "CHO %d GIAY", i);
    drawString(countText, 1);
    delay(1000);
  }
  
  failCount = 0;  // Reset số lần thất bại
  
  // Reset các trạng thái và RFID reader
  isCardDetected = false;
  waitingForFingerprint = false;
  
  // Reset RFID reader
  resetRFIDReader();
  
  drawString("MOI QUET THE", 1);
}

// Khởi tạo hệ thống
void setup() {
  // Khởi tạo OLED
  if (!display.begin(0x3C, true)) {
    while (1); // Lỗi OLED
  }
  display.clearDisplay();
  
  // Khởi tạo giao tiếp
  SPI.begin();
  mfrc522.PCD_Init();
  tranFor.begin(9600);
  mySerial.begin(57600);
  
  // Cấu hình LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // Tắt LED (active LOW)

  // Kiểm tra cảm biến vân tay
  if (!finger.verifyPassword()) {
    drawString("LOI CAM BIEN VAN TAY", 1);
    while (1);
  }

  drawString("HE THONG DANG KHOI DONG", 1);
  delay(DISPLAY_MESSAGE_TIME);
  drawString("CAM BIEN SAN SANG", 1);
  delay(DISPLAY_MESSAGE_TIME);
  drawString("MOI BAN QUET THE", 1);
}

// Vòng lặp chính
void loop() {
  if (!isCardDetected) {
    checkRFID(); // Chờ quét thẻ
  } else if (waitingForFingerprint) {
    // Kiểm tra timeout cho vân tay
    if (millis() - fingerprintTimeLimit >= FINGERPRINT_TIMEOUT) {
      drawString("HET THOI GIAN QUET", 1);
      delay(DISPLAY_MESSAGE_TIME);
      drawString("MOI BAN QUET THE", 1);
      isCardDetected = false;
      waitingForFingerprint = false;
      failCount = 0;  // Reset lại số lần thất bại
      
      // Reset RFID để sẵn sàng đọc thẻ mới
      resetRFIDReader();
      return;
    }

    // Xử lý vân tay nếu phát hiện
    int fingerStatus = getFingerprintID();
    if (fingerStatus != -1) {
      processFingerprint();
    }
    
    // Thêm delay nhỏ để giảm tải CPU
    delay(50);
  }
  
  // Kiểm tra quá nhiều lần thất bại
  if (failCount >= MAX_FAIL_ATTEMPTS) {
    handleFailedAttempts();
  }
}