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
