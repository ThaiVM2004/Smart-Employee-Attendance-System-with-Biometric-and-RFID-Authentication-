# Smart-Employee-Attendance-System-with-Biometric-and-RFID-Authentication-

## 🔧 Description
A smart employee attendance system that combines RFID, fingerprint, and facial recognition technologies. Built on a custom-designed 2-layer PCB, the system integrates multiple embedded platforms (ESP32, STM32, ESP32-CAM) to perform real-time data acquisition, cloud logging, and hardware-based user interaction.
## 🧩 Features
### Multi-factor Authentication:
- RFID tag scanning
- Fingerprint recognition
- Facial image capture via ESP32-CAM
### Google Sheets Integration:
Attendance logs are sent via HTTP to a Google Sheet using the ESP32.
### On-device Storage:
Captured images are saved locally on an SD card in ESP32-CAM.
### Admin Console via Serial Monitor:
- Register new employees
- List registered employees
- Delete employees
## 📐 Hardware Overview
- ESP32 WROM32 DEV KIT              
- STM32F103C8T6 DEV KIT          
- ESP32-CAM DEV KIT (CAM + SD card)         
- OLED Display       
- RFID Module        
- Fingerprint Sensor 
- USB Type-C (SMD)   
- Capacitors
- Adapter 5V-3A
## 🛠️ PCB Design
Designed in EasyEDA, routed as a 2-layer PCB.
- Includes both SMD and through-hole components:
  - Demonstrates dual-soldering skills (precision + assembly).
- Layout considerations:
  - Signal integrity, power routing, decoupling caps.
  - Clear silkscreen, test points, and USB-C footprint.
## Image of PCB:
- Top layer Image:
  
![image](https://github.com/user-attachments/assets/f3ec83b0-9f6f-4ade-adb3-8f3c53b06f05)
- Bottom layer Image:
  
![image](https://github.com/user-attachments/assets/d4364c09-b7e4-438d-b341-7fda7608e5be)

## 🔗 Communication Protocols Used
- UART: STM32, ESP32, ESP32-CAM, Fingerprint Sensor
- I2C: OLED display (SH1106)
- SPI: RFID 
- HTTP: Uploading logs to Google Sheets
## System Overview
This project is a smart attendance system using STM32 as the main controller, integrating fingerprint, RFID, OLED, and communication with ESP32 and ESP32-CAM for data logging and photo capture. The workflow is as follows:

### Startup and Sensor Initialization
On boot, STM32 checks the status of all connected sensors: OLED, fingerprint sensor, and RFID reader.

OLED displays "He thong dang khoi dong..." during initialization.

If all sensors are successfully initialized:

OLED displays "CAM BIEN SAN SANG".

If any sensor fails:

OLED displays "LOI CAM BIEN".

### RFID Authentication
OLED prompts: "MOI QUET THE".

User scans their RFID card.

Card UID (4 bytes in hexadecimal) is sent from STM32 → ESP32 → ESP32-CAM.

ESP32-CAM checks if the UID exists in the SD card database:

If found: returns the user’s ID and full name.

If not found: OLED displays "KHONG TIM THAY".

### Fingerprint Verification
If a valid RFID card is found, the system proceeds to fingerprint verification.

User has 3 attempts to match the fingerprint.

On 3 failed attempts, system enforces a 15-second timeout, then restarts from RFID scan.

If matched:

OLED displays "DIEM DANH THANH CONG".

STM32 sends command e"x" (where x = user ID) to ESP32 → ESP32-CAM.

ESP32-CAM:

Captures a photo using the camera.

Saves the photo to the SD card.

Updates the corresponding user info on Google Sheets.

### Fingerprint Enrollment via Serial
Enrollment is done via serial monitor connected to ESP32 from a computer.

User enters D or d to start registration.

STM32 prompts for ID input via OLED.

ID is entered from the computer through ESP32 serial.

The fingerprint sensor collects two matching scans:

If matched, registration is successful.

If a fingerprint is already registered under the same ID, it will be overwritten.

When a user is deleted from the system, their fingerprint is also removed.

### Admin Commands via ESP32 Serial Monitor
Admin can interact with the system through serial commands on ESP32:

l: List all registered employees from SD card.

a"x": Add a new employee with ID x, followed by name and 4 RFID hex bytes.

g x: Retrieve and display employee data by ID.

d x: Delete employee by ID (also deletes associated fingerprint).

e x: Capture photo of employee with ID x and update Google Sheets.
