#include "esp_camera.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h>

const char* googleScriptId = "https://script.google.com/macros/s/AKfycbx5QKuQZAvtdk5Q6iZ9LmWkVbuHb3sVTyVAgXXFDLXu2BUw-lPJx8uBxhpI-P928gM/exec";

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "Minh Thai";
const char* password = "01202728759";

// REPLACE WITH YOUR TIMEZONE STRING
String myTimezone = "CST-7";

struct User {
    int id;
    String name;
    byte rfid[4];
};

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Stores the camera configuration parameters
camera_config_t config;

// Initializes the camera
void configInitCamera(){
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; //YUV422,GRAYSCALE,RGB565,JPEG
  config.grab_mode = CAMERA_GRAB_LATEST;

  // Select lower framesize if the camera doesn't support PSRAM
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10; //0-63 lower number means higher quality
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Initialize the Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

// Connect to wifi
void initWiFi(){
  WiFi.begin(ssid, password);
  Serial.println("Connecting Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected");
}

// Function to set timezone
void setTimezone(String timezone){
  Serial.printf("  Setting Timezone to %s\n", timezone.c_str());
  setenv("TZ", timezone.c_str(), 1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

// Connect to NTP server and adjust timezone
void initTime(String timezone){
  struct tm timeinfo;
  Serial.println("Setting up time");
  configTime(0, 0, "pool.ntp.org");    // First connect to NTP server, with 0 TZ offset
  if(!getLocalTime(&timeinfo)){
    Serial.println(" Failed to obtain time");
    return;
  }
  Serial.println("Got the time from NTP");
  // Now we can set the real timezone
  setTimezone(timezone);
}

// Get the picture filename based on the current time
String getPictureFilename(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "";
  }
  char timeString[20];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d_%H-%M-%S", &timeinfo);
  Serial.println(timeString);
  String filename = "/picture_" + String(timeString) + ".jpg";
  return filename; 
}

// Initialize the micro SD card
void initMicroSDCard(){
  // Start Micro SD card
  Serial.println("Starting SD Card");
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }
  else {
    Serial.println("SD ok!!");
  }
}

String createDateFolder() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "";
  }
  char folderName[20];
  strftime(folderName, sizeof(folderName), "/%Y-%m-%d", &timeinfo);
  
  fs::FS &fs = SD_MMC;
  if(!fs.mkdir(folderName)){
    Serial.println("Folder exists or creation failed");
  }
  return String(folderName);
}

// Hàm tạo tên file ảnh với ID
String getPictureFilename(int id){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "";
  }
  char timeString[30];
  strftime(timeString, sizeof(timeString), "%H-%M-%S", &timeinfo);
  String folderPath = createDateFolder();
  String filename = folderPath + "/ID" + String(id) + "_" + String(timeString) + ".jpg";
  return filename; 
}

// Hàm chụp và lưu ảnh với ID
bool takeSavePhoto(int id) {
  camera_fb_t * fb = esp_camera_fb_get();
  
  if(!fb) {
    Serial.println("Camera capture failed");
    return false;
  }

  // Lấy thời gian hiện tại
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    esp_camera_fb_return(fb);
    return false;
  }
  
  // Format date và time
  char dateStr[11];
  char timeStr[9];
  strftime(dateStr, sizeof(dateStr), "%d/%m/%Y", &timeinfo);
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  
  // Lấy thông tin người dùng từ ID
  User user = getUserInfo(id);
  // Lưu ảnh vào SD card
  String path = getPictureFilename(id);
  fs::FS &fs = SD_MMC; 
  File file = fs.open(path.c_str(), FILE_WRITE);
  if(!file){
    Serial.printf("Failed to open file in writing mode");
    esp_camera_fb_return(fb);
    return false;
  } 
  else {
    file.write(fb->buf, fb->len);
    Serial.printf("Saved: %s\n", path.c_str());
    
    // Gửi dữ liệu lên Google Sheets
    bool success = sendToGoogleSheets(String(dateStr), String(timeStr), id, user.name);
    file.close();
    esp_camera_fb_return(fb);
    return success;
  }
}

// Hàm đọc thông tin người dùng từ file users.txt
// Hàm đọc thông tin người dùng từ file users.txt
User getUserInfo(int searchId) {
  User user;
  user.id = searchId;
  user.name = "Unknown";
  
  File file = SD_MMC.open("/users.txt", FILE_READ);
  if(!file){
    Serial.println("Failed to open users file");
    return user;
  }
  
  // Bỏ qua header
  String header = file.readStringUntil('\n');
  
  while(file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if(line.length() == 0) continue;
    
    // Phân tích dòng dữ liệu
    int firstComma = line.indexOf(',');
    if(firstComma <= 0) continue;
    
    int id = line.substring(0, firstComma).toInt();
    if(id == searchId) {
      int secondComma = line.indexOf(',', firstComma + 1);
      if(secondComma > 0) {
        user.name = line.substring(firstComma + 1, secondComma);
        user.name.trim();
        
        // Đọc 4 byte RFID
        int start = secondComma + 1;
        for(int i = 0; i < 4; i++) {
          int commaPos = line.indexOf(',', start);
          String byteStr;
          
          if(i == 3 || commaPos == -1) {
            byteStr = line.substring(start);
          } else {
            byteStr = line.substring(start, commaPos);
            start = commaPos + 1;
          }
          
          byteStr.trim();
          if(byteStr.startsWith("0x") || byteStr.startsWith("0X")) {
            byteStr = byteStr.substring(2);
          }
          
          // Chuyển đổi từ hex string sang byte
          char* endPtr;
          user.rfid[i] = strtol(byteStr.c_str(), &endPtr, 16);
        }
      }
      file.close();
      return user;
    }
  }
  
  file.close();
  return user;
}

// Hàm trả về thông tin người dùng dưới dạng chuỗi
String getUserInfoString(int id) {
  User user = getUserInfo(id);
  if(user.name == "Unknown") {
    return "User not found";
  }
  
  String result = String(user.id) + "," + user.name;
  for(int i = 0; i < 4; i++) {
    result += ",0x" + String(user.rfid[i], HEX);
  }
  
  return result;
}


bool sendToGoogleSheets(String date, String time, int id, String name) {
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return false;
  }
  
  HTTPClient http;
  String url = String(googleScriptId);
  
  // Tạo JSON data
  String jsonData = "{\"date\":\"" + date + "\",\"time\":\"" + time + 
                   "\",\"id\":\"" + String(id) + "\",\"name\":\"" + name + "\"}";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST(jsonData);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    Serial.println(response);
    http.end();
    return true;
  }
  else {
    Serial.println("Error on sending POST: " + String(httpResponseCode));
    http.end();
    return false;
  }
}

// Hàm thêm người dùng mới vào file users.txt
bool addNewUser(int id, String name, byte rfid[4]) {
  // Kiểm tra xem ID đã tồn tại chưa
  User existingUser = getUserInfo(id);
  if(existingUser.name != "Unknown") {
    Serial.println("User ID already exists");
    return false;
  }
  
  // Mở file để ghi thêm
  File file = SD_MMC.open("/users.txt", FILE_APPEND);
  if(!file) {
    // Nếu file không tồn tại, tạo file mới với header
    file = SD_MMC.open("/users.txt", FILE_WRITE);
    if(!file) {
      Serial.println("Failed to create users file");
      return false;
    }
    file.println("ID,Name,RFID");
  }
  
  // Ghi thông tin người dùng mới
  String userData = String(id) + "," + name;
  for(int i = 0; i < 4; i++) {
    userData += ",0x" + String(rfid[i], HEX);
  }
  
  file.println(userData);
  file.close();
  Serial.println("User added successfully: " + userData);
  return true;
}

// Hàm phân tích chuỗi RFID từ lệnh
bool parseRFID(String rfidStr, byte rfid[4]) {
  int start = 0;
  int byteCount = 0;
  
  // Đếm số lượng dấu phẩy để kiểm tra format
  int commaCount = 0;
  for (int i = 0; i < rfidStr.length(); i++) {
    if (rfidStr.charAt(i) == ',') {
      commaCount++;
    }
  }
  
  // RFID hợp lệ phải có đúng 3 dấu phẩy (4 byte)
  if (commaCount != 3) {
    Serial.println("Invalid RFID format: Must have exactly 4 bytes separated by commas");
    return false;
  }
  
  for(int i = 0; i < 4; i++) {
    int commaPos = rfidStr.indexOf(',', start);
    String byteStr;
    
    if(i == 3 || commaPos == -1) {
      byteStr = rfidStr.substring(start);
    } else {
      byteStr = rfidStr.substring(start, commaPos);
      start = commaPos + 1;
    }
    
    byteStr.trim();
    if(byteStr.startsWith("0x") || byteStr.startsWith("0X")) {
      byteStr = byteStr.substring(2);
    }
    
    // Kiểm tra xem byteStr có phải là giá trị hex hợp lệ không
    for (int j = 0; j < byteStr.length(); j++) {
      char c = byteStr.charAt(j);
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
        Serial.println("Invalid hex character in RFID: " + byteStr);
        return false;
      }
    }
    
    // Chuyển đổi từ hex string sang byte
    char* endPtr;
    rfid[i] = strtol(byteStr.c_str(), &endPtr, 16);
    byteCount++;
  }
  
  // Đảm bảo đủ 4 byte
  if(byteCount != 4) {
    Serial.println("Invalid RFID: Must have exactly 4 bytes");
    return false;
  }
  
  return true;
}


// Hàm liệt kê tất cả người dùng từ file users.txt
void listAllUsers() {
  File file = SD_MMC.open("/users.txt", FILE_READ);
  if(!file) {
    Serial.println("Failed to open users file or file doesn't exist");
    return;
  }
  
  // Bỏ qua header
  String header = file.readStringUntil('\n');
  
  // In tiêu đề
  Serial.println("ID,Name,RFID");
  Serial.println("------------------");
  
  int userCount = 0;
  String line;
  
  while(file.available()) {
    line = file.readStringUntil('\n');
    line.trim();
    if(line.length() == 0) continue;
    
    // In thông tin người dùng
    Serial.println(line);
    userCount++;
  }
  
  file.close();
  
  // In tổng số người dùng
  Serial.println("------------------");
  Serial.println("Total users: " + String(userCount));
}


// Hàm xóa người dùng theo ID
// Hàm xóa người dùng theo ID - Phiên bản cải tiến
bool deleteUser(int id) {
  // Kiểm tra xem ID có tồn tại không
  User existingUser = getUserInfo(id);
  if(existingUser.name == "Unknown") {
    Serial.println("User ID does not exist");
    return false;
  }
  
  // Tạo file tạm
  File sourceFile = SD_MMC.open("/users.txt", FILE_READ);
  File tempFile = SD_MMC.open("/temp.txt", FILE_WRITE);
  
  if(!sourceFile || !tempFile) {
    Serial.println("Failed to open files");
    if(sourceFile) sourceFile.close();
    if(tempFile) tempFile.close();
    return false;
  }
  
  // Đọc header và ghi vào file tạm
  String header = sourceFile.readStringUntil('\n');
  tempFile.println(header);
  
  bool userFound = false;
  
  // Đọc từng dòng và ghi lại vào file tạm, ngoại trừ dòng cần xóa
  while(sourceFile.available()) {
    String line = sourceFile.readStringUntil('\n');
    line.trim();
    if(line.length() == 0) continue;
    
    // Phân tích dòng dữ liệu để lấy ID
    int firstComma = line.indexOf(',');
    if(firstComma <= 0) {
      tempFile.println(line); // Ghi lại dòng không đúng định dạng
      continue;
    }
    
    int currentId = line.substring(0, firstComma).toInt();
    if(currentId != id) {
      // Giữ lại người dùng khác
      tempFile.println(line);
    } else {
      userFound = true;
    }
  }
  
  sourceFile.close();
  tempFile.close();
  
  // Xóa file gốc và đổi tên file tạm
  if(userFound) {
    SD_MMC.remove("/users.txt");
    SD_MMC.rename("/temp.txt", "/users.txt");
    Serial.println("User with ID " + String(id) + " deleted successfully");
    return true;
  } else {
    SD_MMC.remove("/temp.txt");
    Serial.println("User ID not found in file");
    return false;
  }
}


User findUserByRFID(byte targetRfid[4]) {
  User user;
  user.id = -1;
  user.name = "Unknown";
  memset(user.rfid, 0, 4);

  File file = SD_MMC.open("/users.txt", FILE_READ);
  if(!file) {
    Serial.println("Failed to open users file");
    return user;
  }

  // Bỏ qua header
  file.readStringUntil('\n');

  while(file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if(line.length() == 0) continue;

    int firstComma = line.indexOf(',');
    if(firstComma <= 0) continue;

    int secondComma = line.indexOf(',', firstComma + 1);
    if(secondComma <= 0) continue;

    String rfidPart = line.substring(secondComma + 1);
    byte currentRfid[4];
    if(!parseRFID(rfidPart, currentRfid)) continue;

    // So sánh RFID
    bool match = true;
    for(int i = 0; i < 4; i++) {
      if(currentRfid[i] != targetRfid[i]) {
        match = false;
        break;
      }
    }

    if(match) {
      user.id = line.substring(0, firstComma).toInt();
      user.name = line.substring(firstComma + 1, secondComma);
      user.name.trim();
      memcpy(user.rfid, currentRfid, 4);
      file.close();
      return user;
    }
  }
  
  file.close();
  return user;
}




// Xử lý lệnh UART
void processCommand(String command) {
  if(command.length() < 1) {
    Serial.println("Invalid command format");
    return;
  }
  
  char cmd = command.charAt(0);
  int id = command.substring(1).toInt();
  
  switch(cmd) {
    case 'G':
    case 'g':
      // Trường hợp 1: Trả về thông tin người dùng
      Serial.println(getUserInfoString(id));
      break;
      
    case 'E':
    case 'e':
      // Trường hợp 2: Chụp ảnh và tải lên Google Sheets
      if(takeSavePhoto(id)) {
        Serial.println("OK");
      } else {
        Serial.println("ER");
      }
      break;
  case 'A':
  case 'a': {
    // Trường hợp 3: Thêm người dùng mới
    // Format: A<ID>,<Name>,<RFID_byte1>,<RFID_byte2>,<RFID_byte3>,<RFID_byte4>
    String data = command.substring(1);
    int firstComma = data.indexOf(',');
    if(firstComma <= 0) {
      Serial.println("Invalid format. Use: A<ID>,<Name>,<RFID1>,<RFID2>,<RFID3>,<RFID4>");
      return;
    }
    
    int id = data.substring(0, firstComma).toInt();
    if (id <= 0) {
      Serial.println("Invalid ID: Must be a positive number");
      return;
    }
    
    int secondComma = data.indexOf(',', firstComma + 1);
    if(secondComma <= 0) {
      Serial.println("Invalid format. Use: A<ID>,<Name>,<RFID1>,<RFID2>,<RFID3>,<RFID4>");
      return;
    }
    
    String name = data.substring(firstComma + 1, secondComma);
    name.trim();
    if (name.length() == 0) {
      Serial.println("Name cannot be empty");
      return;
    }
    
    String rfidStr = data.substring(secondComma + 1);
    byte rfid[4];
    
    if(!parseRFID(rfidStr, rfid)) {
      Serial.println("Invalid RFID format. Use: 0xXX,0xXX,0xXX,0xXX");
      return;
    }
    
    if(addNewUser(id, name, rfid)) {
      Serial.println("OK");
    } else {
      Serial.println("ER");
    }
    break;
  }

    case 'L':
    case 'l': 
      // Trường hợp 4: Liệt kê tất cả người dùng
      listAllUsers();
      break;
  case 'D':
    case 'd':
      // Trường hợp 5: Xóa người dùng theo ID
      if(deleteUser(id)) {
        Serial.println("OK");
      } else {
        Serial.println("ER");
      }
      break;
      default: // Xử lý RFID không có command
      byte rfid[4];
      if(parseRFID(command, rfid)) {
        User user = findUserByRFID(rfid);
        if(user.id != -1) {
          Serial.println(String(user.id) + "," + user.name);
        } else {
          Serial.println("User not found");
        }
      } else {
        Serial.println("Unknown command");
      }
      break;
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  //Serial1.begin(115200, SERIAL_8N1, 14, 15);
  
  // Khởi tạo WiFi
  initWiFi();
  // Khởi tạo thời gian
  initTime(myTimezone);
  // Khởi tạo camera
  configInitCamera();
  // Khởi tạo SD Card
  initMicroSDCard();
  Serial.println("System initialized and ready");
}

void loop() {
  if(Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    processCommand(input);
  }
  delay(100);
}