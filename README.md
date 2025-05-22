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
