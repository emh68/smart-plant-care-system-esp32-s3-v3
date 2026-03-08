# Overview

This project refactors a basic Arduino watering setup into a professional, object-oriented system using C++ and PlatformIO. By moving from procedural scripts to custom classes, I developed a scalable framework for managing asynchronous sensor data and complex UI states.

The system monitors 10-channel spectral light, soil moisture, and environmental data. It features an interactive OLED menu driven by a Finite State Machine (FSM) and an I2C rotary encoder. Key technical implementations include non-volatile memory (NVS) for persistent calibration and manual hardware overrides.

[Software Demo Video](https://youtu.be/OUlXXAMPLh4)

# Development Environment

* **IDE:** Visual Studio Code with PlatformIO extension.
* **Hardware:** Seeed Studio XIAO ESP32-S3 Microcontroller.
* **Communication Protocol:** I2C (Inter-Integrated Circuit) for sensor bus and encoder.

### Programming Language and Libraries
* **Language:** C++ (Arduino Framework).
* **Libraries:** 
    * `U8g2lib.h` (OLED rendering)
    * `Wire.h` (I2C communication)
    * `Preferences.h` (Flash memory/File I/O stretch goal)
    * `NTPClient.h` & `WiFi.h` (Network time synchronization)
    * `Seeed_SHT35.h` (Environmental sensing)
    * `Adafruit_AS7341.h` (Spectral Analysis)
    * `Grove_Moisture_Sensor.h` (DFRobot/Grove Moisture Logic)
    * `Grove_MOSFET.h` (Pump driver control)

### Required Hardware
* **Microcontroller:** Seeed Studio XIAO ESP32-S3.
* **Sensors:** SHT35 (Temp/Hum), AS7341 (10-Channel Spectral), DFRobot Capacitive Soil Moisture (SEN0193).
* **Actuators:** Seeed Grove MOSFET & DC Water Pump.
* **Display/Input:** SSD1315 OLED (128x64), M5Stack Unit Encoder.

## Environment Setup & Installation

This guide explains how to set up the development environment and deploy the Smart Plant Care system to a Seeed Studio XIAO ESP32-S3 using PlatformIO.

---

### 1. Install Required Tools

#### Install Visual Studio Code
Download and install Visual Studio Code:

https://code.visualstudio.com/

#### Install PlatformIO
1. Open **VS Code**.
2. Click the **Extensions** icon on the left sidebar (`Ctrl + Shift + X`).
3. Search for **PlatformIO IDE**.
4. Click **Install**.

PlatformIO manages the ESP32 toolchain, libraries, and firmware uploads.

---

### 2. Project Configuration

Your project must contain a **`platformio.ini`** file in the root directory.  
This file defines the board configuration and required libraries.

Create or update `platformio.ini` with the following:

```ini
; Please visit documentation for the other options and examples
; https://docs.platformio.org/page/projectconf.html
; PlatformIO Configuration File for Xiao ESP32-S3
; ------------------------------------------------
[env:seeed_xiao_esp32s3]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino
monitor_speed = 115200

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1

lib_deps =
    https://github.com/Seeed-Studio/Grove_High_Precision_RTC_PCF85063TP.git
    https://github.com/Seeed-Studio/Seeed_SHT35.git
    olikraus/U8g2 @ ^2.35.9
    arduino-libraries/NTPClient @ ^3.2.1
    adafruit/Adafruit AS7341 @ ^1.4.1
```
### 3. Hardware Wiring

Connect the system components to the **XIAO ESP32-S3** as described below.

---

#### I2C Devices

All I2C devices share the same bus.

| Device Pin | XIAO Pin |
|-----------|-----------|
| SDA | GPIO 5 |
| SCL | GPIO 6 |

Connect the following devices to the I2C bus:

- OLED display  
- SHT35 temperature and humidity sensor  
- PCF85063TP RTC  
- Any additional I2C sensors

---

#### Soil Moisture Sensor

DFRobot Capacitive Soil Moisture Sensor
>*Note:* This sensor must use an analog pin on the microcontroller (A1, A2, etc.)

| Sensor Pin | XIAO Pin |
|-----------|-----------|
| SIG | A2 (GPIO 3) |
| VCC | 3V3 |
| GND | GND |

---

#### Pump / MOSFET Driver

Seeed Grove MOSFET or Pump Driver

| Driver Pin | XIAO Pin |
|-----------|-----------|
| SIG | D10 (GPIO 9) |
| VCC | 3V3 |
| GND | GND |

The **SIG pin** controls the watering pump (on/off).

### 4. Wi-Fi Configuration
Create a file named:
```txt
include/secrets.h
```
Add your Wi-Fi credentials:
```c++
#define WIFI_SSID "Your_Network_Name"
#define WIFI_PASS "Your_Password"
```
Keeping credentials in this file prevents them from being committed to version control.

### 5. Build & Upload Firmware

#### Connect the Board
Plug the `XIAO ESP32-S3` into your computer using a USB-C cable.

#### Build the Project
Click the **✓ (Build)** icon in the VS Code status bar.

PlatformIO will:
- Download required libraries
- Compile the firmware

#### Upload the Firmware
Click the **→ (Upload)** icon to flash the firmware to the board.

#### Open the Serial Monitor
Click the **plug icon** to open the **Serial Monitor**.

#### On Startup the System Will
- Boot the ESP32
- Connect to Wi-Fi
- Synchronize the time using `NTP`
- Begin monitoring sensors and controlling plant watering

# Useful Websites

### Hardware & Sensors
* [XIAO ESP32-S3 Getting Started](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
* [M5Stack Unit Encoder Product Page](https://docs.m5stack.com/en/unit/encoder)
* [Adafruit AS7341 10-Channel Light Sensor Wiki](https://learn.adafruit.com/adafruit-as7341-10-channel-light-color-sensor-breakout/arduino)
* [DFRobot Capacitive Soil Moisture Sensor (SEN0193)](https://wiki.dfrobot.com/sen0193/)
* [Seeed Studio SHT35 Temp/Hum Sensor](https://wiki.seeedstudio.com/Grove-I2C_High_Accuracy_Temp%2526Humi_Sensor-SHT35/)
* [Seeed Studio Grove MOSFET (Pump Control)](https://wiki.seeedstudio.com/Grove-MOSFET/)
* [Adafruit 3V DV Water Pump (submersible horizontal)](https://www.adafruit.com/product/4546)
* [Grove DS1307 RTC (Real Time Clock)](https://wiki.seeedstudio.com/Grove-RTC/)
* [DFRobot ProtoBoard - Rectangle 2" Single Sided](https://www.dfrobot.com/product-388.html?gad_source=1&gad_campaignid=23441887437&gbraid=0AAAAADucPlCHdn7-8BKVYNYW9_WjpTCbm&gclid=CjwKCAiAtq_NBhA_EiwA78nNWKz9YP4DaKY4bCC-SkvS-mBcr7qkboHzVxxd0d49X3hYWCc7OZP9hRoCycoQAvD_BwE)
* [M5 Stack Grove to 4 Pin](https://shop.m5stack.com/products/connector-grove-to-4-pin-10pcs?srsltid=AfmBOoo6XyI0oYHN5GN71C4pX0s3Lb00qFoHPAAia5kM9i9gnUL140Nr)
* [Seeed Studio Grove - OLED Display .96" (SSD1315)](https://wiki.seeedstudio.com/Grove-OLED-Display-0.96-SSD1315/)


### Technical References & Source Code
* [M5Unit-Encoder GitHub (Unit_Encoder.cpp)](https://github.com/m5stack/M5Unit-Encoder/blob/master/src/Unit_Encoder.cpp)
* [M5Unit-Encoder GitHub (Unit_Encoder.h)](https://github.com/m5stack/M5Unit-Encoder/blob/master/src/Unit_Encoder.h)
* [Adafruit_AS7341 GitHub Library](https://github.com/adafruit/Adafruit_AS7341)
* [Seeed_SHT35 GitHub Library](https://github.com/Seeed-Studio/Seeed_SHT35/blob/master/Seeed_SHT35.h)
* [Grove High Precision RTC (PCF85063TP)](https://github.com/Seeed-Studio/Grove_High_Precision_RTC_PCF85063TP.git)
* [OLED Display .96" (SSD1315) - u8g2 Library & Docs](https://github.com/olikraus/u8g2?tab=readme-ov-file)
* [W3Schools C++ Tutorial](https://www.w3schools.com/cpp/default.asp)
* [GeeksforGeeks C++ Programming](https://www.geeksforgeeks.org/cpp/c-plus-plus/)
* *C++ For Everyone 2nd Edition* By Cay Horstmann.

# Future Work
* **Dynamic Calibration:** Implement a UI-based calibration routine to set "Dry" and "Wet" points via the encoder.
* **WiFi Manager:** Replace hardcoded credentials with an Access Point (AP) mode for dynamic configuration.
* **Historical Logging:** Log sensor data over time to an SD card for long-term trend analysis.


## AI Disclosure
AI tools were used only for assistance with development environment setup (PlatformIO) and general hardware troubleshooting guidance. All firmware implementation, application logic, and project source code were written by me.