# Overview

As a software engineer, I am focused on evolving this project from a local hardware controller into a connected IoT (Internet of Things) ecosystem. This sprint was dedicated to integrating a cloud-based back-end to manage complex plant data and historical logging, moving beyond the memory limitations of a standalone microcontroller.

The software is a real-time plant monitoring and automated watering system. It synchronizes with a Firebase Realtime Database to pull specific care profiles (moisture, temperature, and humidity thresholds) for 13 different plant species. It provides an interactive UI on a high-resolution TFT display, allowing users to select profiles, monitor live spectral light data, and manage cloud logs.

The purpose of this software is to demonstrate the bridge between embedded C++ and cloud infrastructure. By offloading plant care requirements to a database, the system becomes dynamic—allowing for remote updates to plant care logic without requiring a firmware re-flash.

[Software Demo Video](placeholder)


## Cloud Database
* `/plantLibrary` A collection of objects where each key is a unique plant ID (e.g., fiddle-leaf-fig). Each object contains integer and string values for moisture, temperature, humidity, and light requirements.

* `/plantIndex` A lightweight array of all available plant IDs, used to efficiently populate the device's selection menu.

* `/sensorData` A write-only node where the ESP32 creates new entries every 30 seconds, logging timestamps, soil moisture, environmental metrics, and pump activation status.

### Firebase Data Snippet
```json
{
  "plantIndex": [
    "aloe-vera",
    "basil",
    "fiddle-leaf-fig"
  ],
  "plantLibrary": {
    "aloe-vera": {
      "humidityMax": 50,
      "lightNeeds": "Full sun to bright light",
      "moistureMin": 15,
      "plantName": "Aloe Vera",
      "soilPhMax": 7.5,
      "tempMax": 80,
      "wateringFrequencyDays": 14
    },
    "fiddle-leaf-fig": {
      "humidityMax": 65,
      "lightNeeds": "Bright, filtered light",
      "moistureMin": 35,
      "plantName": "Fiddle Leaf Fig",
      "soilPhMax": 7,
      "tempMax": 85,
      "wateringFrequencyDays": 7
    }
  },
  "sensorData": {
    "-OKjL2xYz...": {
      "plantId": "fiddle-leaf-fig",
      "plantName": "Fiddle Leaf Fig",
      "soilMoisture": 42,
      "timestamp": "03/28/2026 10:45 PM",
      "pumpActivated": false
    }
  }
}
```

## Development Environment

### Required Hardware
* **Microcontroller:** Seeed Studio XIAO ESP32-S3.
* **Sensors:** <br>SHT35 (Temp/Hum)<br>AS7341 (10-Channel Spectral)<br>DFRobot Capacitive Soil Moisture (SEN0193).
* **Solenoid Driver:** Adafruit MCP23017 GPIO Expander & solenoid driver 
* **Pump:** 3v DC Submersible Water Pump.
* **Display:** Adafruit 2.0" TFT IPS 240 x 320 ST7789 Display
* **18 Pin EYE SPI Breakout Board:** Used to connect the display to SPI on microcontroller
* **Input/Rotary Encoder:** U135 M5Stack Rotary Encoder Unit.
* **IDE:** Visual Studio Code with PlatformIO extension.
* **Communication Protocol:** I2C (Inter-Integrated Circuit) for sensor bus and encoder.

### Programming Language and Libraries
* **Language:** C++ (Arduino Framework).
* **Libraries:** 
    * `Adafruit_ST7789.h` (2.0" TFT IPS Display driver)
    * `Adafruit_GFX` (Graphics library)
    * `Wire.h` (I2C communication)
    * `Preferences.h` (Flash memory/File I/O stretch goal)
    * `NTPClient.h` & `WiFi.h` (Network time synchronization)
    * `Seeed_SHT35.h` (Environmental sensing)
    * `Adafruit_AS7341.h` (Spectral Analysis)
    * `Grove_Moisture_Sensor.h` (DFRobot/Grove Moisture Logic)
    * `Adafruit_MCP23X17.h` (MCP23017 GPIO Expander & 8 Channel Solenoid Driver)
    * `Firebase_ESP_Client.h` (Firebase Realtime Database)
    * `Adafruit_ADS1X15.h` (Not used in this version. In future version allows multiple soil moisture sensors for multiple plants)

## Environment Setup & Installation

This guide explains how to set up the development environment and deploy the Smart Plant Care system to a Seeed Studio XIAO ESP32-S3 using PlatformIO.

### 1. Install Required Tools

---

#### Install Visual Studio Code
Download and install Visual Studio Code:

https://code.visualstudio.com/

#### Install PlatformIO
1. Open **VS Code**.
2. Click the **Extensions** icon on the left sidebar (`Ctrl + Shift + X`).
3. Search for **PlatformIO IDE**.
4. Click **Install**.

PlatformIO manages the ESP32 toolchain, libraries, and firmware uploads.

### 2. Project Configuration

---

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

monitor_dtr = 0
monitor_rts = 0


lib_deps = 
    https://github.com/Seeed-Studio/Grove_High_Precision_RTC_PCF85063TP.git
    https://github.com/Seeed-Studio/Seeed_SHT35.git
    arduino-libraries/NTPClient @ ^3.2.1
    adafruit/Adafruit AS7341 @ ^1.4.1
    adafruit/Adafruit BusIO @ ^1.16.1
    adafruit/Adafruit ADS1X15 @ ^2.6.2
    https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library.git
    adafruit/Adafruit GFX Library @ ^1.12.5
    https://github.com/adafruit/Adafruit-ST7735-Library.git
    mobizt/Firebase Arduino Client Library for ESP8266 and ESP32 @ ^4.4.14

```
### 3. Hardware Wiring

---

Connect the system components to the **XIAO ESP32-S3** as described below.

### I2C Devices

All I2C devices share the same bus.

| Device Pin | XIAO Pin |
|-----------|-----------|
| SDA | GPIO 5 |
| SCL | GPIO 6 |

Connect the following devices to the I2C bus:
  
- SHT35 temperature and humidity sensor  
- PCF85063TP RTC  
- Any additional I2C sensors

---

### Adafruit 2.0" TFT (ST7789) SPI Pinout

| EYESPI / TFT Pin | XIAO ESP32-S3 Pin | Function | Description |
| :--- | :--- | :--- | :--- |
| VIN | 3V3 | Power | 3.3V Power Supply |
| GND | GND | Ground | Common Ground |
| SCK | GPIO 7 (D8) | SCK | SPI Clock Line |
| MOSI | GPIO 9 (D10)| MOSI | Master Out Slave In |
| TCS | GPIO 2 (D1) | TFT_CS | Chip Select |
| DC | GPIO 3 (D2) | TFT_DC | Data/Command Toggle |
| RST | GPIO 4 (D3) | TFT_RST | Hardware Reset |


### Soil Moisture Sensor

DFRobot Capacitive Soil Moisture Sensor
>*Note:* This sensor must use an analog pin on the microcontroller (A1, A2, etc.)

| Sensor Pin | XIAO Pin |
|-----------|-----------|
| SIG | D0 (GPIO 1) |
| VCC | 3V3 |
| GND | GND |

---

### 4. Wi-Fi Configuration

---

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

---

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

## Useful Websites

### Hardware & Sensors
* [XIAO ESP32-S3 Getting Started](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
* [M5Stack Unit Encoder Product Page](https://docs.m5stack.com/en/unit/encoder)
* [Adafruit AS7341 10-Channel Light Sensor Wiki](https://learn.adafruit.com/adafruit-as7341-10-channel-light-color-sensor-breakout/arduino)
* [DFRobot Capacitive Soil Moisture Sensor (SEN0193)](https://wiki.dfrobot.com/sen0193/)
* [Seeed Studio SHT35 Temp/Hum Sensor](https://wiki.seeedstudio.com/Grove-I2C_High_Accuracy_Temp%2526Humi_Sensor-SHT35/)
* [Adafruit MCP23017 I2C to 8 Channel Solenoid Driver (Pump Control)](https://learn.adafruit.com/adafruit-i2c-to-8-channel-solenoid-driver/overview)
* [Adafruit 3V DV Water Pump (submersible horizontal)](https://www.adafruit.com/product/4546)
* [Grove DS1307 RTC (Real Time Clock)](https://wiki.seeedstudio.com/Grove-RTC/)
* [DFRobot ProtoBoard - Rectangle 2" Single Sided](https://www.dfrobot.com/product-388.html?gad_source=1&gad_campaignid=23441887437&gbraid=0AAAAADucPlCHdn7-8BKVYNYW9_WjpTCbm&gclid=CjwKCAiAtq_NBhA_EiwA78nNWKz9YP4DaKY4bCC-SkvS-mBcr7qkboHzVxxd0d49X3hYWCc7OZP9hRoCycoQAvD_BwE)
* [M5 Stack Grove to 4 Pin](https://shop.m5stack.com/products/connector-grove-to-4-pin-10pcs?srsltid=AfmBOoo6XyI0oYHN5GN71C4pX0s3Lb00qFoHPAAia5kM9i9gnUL140Nr)
* [Adafruit 2.0" 240 x 320 Color IPS TFT Display (ST7789 driver)](https://learn.adafruit.com/2-0-inch-320-x-240-color-ips-tft-display)


### Technical References & Source Code
* [XIAO ESP32-S3 Series](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
* [M5Unit-Encoder GitHub (Unit_Encoder.cpp)](https://github.com/m5stack/M5Unit-Encoder/blob/master/src/Unit_Encoder.cpp)
* [M5Unit-Encoder GitHub (Unit_Encoder.h)](https://github.com/m5stack/M5Unit-Encoder/blob/master/src/Unit_Encoder.h)
* [Adafruit_AS7341 GitHub Library](https://github.com/adafruit/Adafruit_AS7341)
* [Seeed_SHT35 GitHub Library](https://github.com/Seeed-Studio/Seeed_SHT35/blob/master/Seeed_SHT35.h)
* [Grove High Precision RTC (PCF85063TP)](https://github.com/Seeed-Studio/Grove_High_Precision_RTC_PCF85063TP.git)
* [Adafruit 2.0" 240 x 320 Color IPS TFT Display ST7789 (It is for ST7735 & ST7789)](https://github.com/adafruit/Adafruit-ST7735-Library)
* [Adafruit EYESPI Breakout Board](https://learn.adafruit.com/adafruit-eyespi-breakout-board/arduino)
* [Mobitz Firebase-ESP-Client GitHub](https://github.com/mobizt/Firebase-ESP-Client)
* [Firebase Realtime Database](https://firebase.google.com/docs/database)
* [W3Schools C++ Tutorial](https://www.w3schools.com/cpp/default.asp)
* [GeeksforGeeks C++ Programming](https://www.geeksforgeeks.org/cpp/c-plus-plus/)
* *C++ For Everyone 2nd Edition* By Cay Horstmann.

## Future Work

* **Migrate to FirebaseClient 1.2.0:** Transition to the newer, memory-efficient library to improve SSL handshake reliability and implement data streaming.
* **Bi-Directional Control:** Implement a "Remote Manual Override" where a value changed in the Firebase console can trigger the water pump remotely.
* **Improve UI:** Refactor the menu and display to support better user experience and viewing different watering zones for multi-plant management.


## AI Disclosure
AI tools were used only for assistance with development environment setup (PlatformIO) and general hardware troubleshooting/debugging guidance. All firmware implementation, application logic, and project source code were written by me.