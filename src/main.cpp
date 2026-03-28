#include <Arduino.h>
#include <Wire.h>
#include "Seeed_SHT35.h"
#include <WiFi.h>
#include "secrets.h"
#include "LightSensor.h"
#include "TempHumSensor.h"
#include "WateringZone.h"
#include "SystemTime.h"
#include "DisplayManager.h"
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// Variables for the on-screen keyboard
const String charset = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*<>";
int charIndex = 0;
String inputBuffer = "";
enum EntryMode
{
  ENTER_SSID,
  ENTER_PASS,
  CALIBRATE_DRY,
  CALIBRATE_WET
};
EntryMode entryMode = ENTER_SSID;

// Sub-menu states for the Settings screen
enum SettingsSubState
{
  SETTINGS_MAIN,
  WIFI_SSID,
  WIFI_PASS,
  CALIB_DRY,
  CALIB_WET,
  RENAME_PLANT
};
SettingsSubState settingsState = SETTINGS_MAIN;
const char *settingsOptions[] = {"[Back]", "WiFi Config", "Calibrate Sensor", "Rename Plant"};
int settingsMenuIndex = 0;

// Soil Moisture values used for 10-second calibration
int tempDryValue = 700;
int tempWetValue = 330;

// TFT Display Pins
#define TFT_CS 2
#define TFT_DC 3
#define TFT_RST 4
Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);

// Sensor and manager objects
TempHumSensor envSensor(18);
LightSensor spectralSensor;
SystemTime sysTime(-7); // Mountain Time Zone
DisplayManager display(&tft);
Adafruit_MCP23X17 mcp;

const char *channelLabels[] = {"F1-415nm", "F2-445nm", "F3-480nm", "F4-515nm", "F5-555nm", "F6-590nm", "F7-630nm", "F8-680nm", "Clear", "NIR"};
WateringZone zones[] = {WateringZone(1, 0, 0, "Plant 1")};

// Main Menu Screen States
enum AppState
{
  DASHBOARD,
  MENU_SCREEN,
  PLANT_VIEW,
  LIGHT_VIEW,
  SETTINGS_VIEW
};
AppState currentState = DASHBOARD;

int menuIndex = 0;
int16_t lastEncoderPos = 0;
uint8_t lastBtnStatus = 0;
bool manualPumpOn = false;

// I2C addresses for Rotary Encoder
const uint8_t ENCODER_ADDR = 0x40;
const uint8_t REG_ENCODER = 0x10;
const uint8_t REG_BUTTON = 0x20;

const char *menuOptions[] = {"[Exit Menu]", "Plant Profile", "Light Levels", "Pump: OFF", "Settings"};

void setup()
{
  Serial.begin(115200);
  delay(2000);

  Wire.begin(5, 6);
  Wire.setClock(100000);

  // Initialize TFT Display & show status
  tft.init(240, 320);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_GREEN);
  tft.print("Hardware Init...");
  delay(1000);

  // TEST RTC & NTP Initialization
  sysTime.begin();
  tft.fillScreen(ST77XX_RED);
  tft.print("Time OK");
  delay(1000);

  // TEST SHT35 Temp & Hum Intialization
  envSensor.init();
  tft.fillScreen(ST77XX_BLUE);
  tft.print("SHT35 OK");
  delay(1000);

  // TEST MCP 23017 Driver Initialization
  mcp.begin_I2C(0x20);
  zones[0].attachMCP(&mcp);
  zones[0].begin();
  tft.fillScreen(ST77XX_MAGENTA);
  tft.print("MCP OK");
  delay(1000);

  // TEST AS7341 Light Sensor Initialization
  if (!spectralSensor.begin())
  {
    Serial.println("SPECTRAL SENSOR NOT FOUND!");
    tft.fillScreen(ST77XX_ORANGE);
    tft.print("Light Sensor Error");
    while (1)
      ;
  }
  else
  {
    Serial.println("Spectral Sensor OK");
  }
}

void loop()
{
  envSensor.update();
  spectralSensor.update();
  zones[0].update();

  // Serial Monitor output for light channels
  for (int i = 0; i < 10; i++)
  {
    Serial.print(">");
    Serial.print(channelLabels[i]);
    Serial.print(":");
    Serial.println(spectralSensor.getChannelValue(i));
  }

  // Serial Monitor output for Soil Moisture
  Serial.print("Plant: ");
  Serial.print(zones[0].getId());
  Serial.print(" | Raw: ");
  Serial.print(analogRead(1));
  Serial.print(" | Moisture: ");
  Serial.print(zones[0].getMoisturePercent());
  Serial.println("%\n");

  // Read Rotary Encoder & Button Depress
  uint8_t currentBtnStatus = 0;
  Wire.beginTransmission(ENCODER_ADDR);
  Wire.write(REG_BUTTON);
  Wire.endTransmission();
  Wire.requestFrom(ENCODER_ADDR, (uint8_t)1);
  if (Wire.available())
  {
    currentBtnStatus = Wire.read();
  }

  if (currentBtnStatus == 1 && lastBtnStatus == 0)
  {
    if (currentState == DASHBOARD)
    {
      currentState = MENU_SCREEN;
    }
    else if (currentState == MENU_SCREEN)
    {
      // Logic for selecting items in main menu
      if (menuIndex == 0)
        currentState = DASHBOARD;
      else if (menuIndex == 1)
        currentState = PLANT_VIEW;
      else if (menuIndex == 2)
        currentState = LIGHT_VIEW;
      else if (menuIndex == 3)
      {
        manualPumpOn = !manualPumpOn;
        if (manualPumpOn)
        {
          zones[0].turnPumpOn();
          menuOptions[3] = "Pump: ON";
        }
        else
        {
          zones[0].turnPumpOff();
          menuOptions[3] = "Pump: OFF";
        }
      }
      else if (menuIndex == 4)
      {
        currentState = SETTINGS_VIEW;
        settingsState = SETTINGS_MAIN;
      }
    }
    else if (currentState == SETTINGS_VIEW)
    {
      if (settingsState == SETTINGS_MAIN)
      {
        if (settingsMenuIndex == 0)
          currentState = MENU_SCREEN;
        else if (settingsMenuIndex == 1)
        {
          settingsState = WIFI_SSID;
          inputBuffer = "";
          charIndex = 0;
        }
        else if (settingsMenuIndex == 2)
        {
          settingsState = CALIB_DRY;
        }
        else if (settingsMenuIndex == 3)
        {
          settingsState = RENAME_PLANT;
        }
      }
      else if (settingsState == WIFI_SSID || settingsState == WIFI_PASS)
      {
        char currentLetter = charset[charIndex];
        if (currentLetter == '>') // Used as Enter
        {
          if (settingsState == WIFI_SSID)
          {
            settingsState = WIFI_PASS;
            inputBuffer = "";
          }
          else
          {
            settingsState = SETTINGS_MAIN;
          }
        }
        else if (currentLetter == '<') // Used as Backspace
        {
          if (inputBuffer.length() > 0)
            inputBuffer.remove(inputBuffer.length() - 1);
        }
        else
        {
          inputBuffer += currentLetter;
        }
      }
    }
    else
    {
      currentState = DASHBOARD;
    }
    delay(200);
  }
  lastBtnStatus = currentBtnStatus;

  // Read Rotary Encoder Rotation
  Wire.beginTransmission(ENCODER_ADDR);
  Wire.write(REG_ENCODER);
  Wire.endTransmission();
  Wire.requestFrom(ENCODER_ADDR, (uint8_t)2);
  if (Wire.available() >= 2)
  {
    uint8_t low = Wire.read();
    uint8_t high = Wire.read();
    int16_t currentPos = (int16_t)(low | (high << 8));
    if (currentPos != lastEncoderPos)
    {
      bool forward = (currentPos > lastEncoderPos);
      if (currentState == MENU_SCREEN)
      {
        menuIndex = forward ? (menuIndex + 1) % 5 : (menuIndex - 1 + 5) % 5;
      }
      else if (currentState == SETTINGS_VIEW)
      {
        if (settingsState == SETTINGS_MAIN)
        {
          settingsMenuIndex = forward ? (settingsMenuIndex + 1) % 4 : (settingsMenuIndex - 1 + 4) % 4;
        }
        else
        {
          charIndex = forward ? (charIndex + 1) % charset.length() : (charIndex - 1 + charset.length()) % charset.length();
        }
      }
      lastEncoderPos = currentPos;
    }
  }

  // Render current screen
  switch (currentState)
  {
  case DASHBOARD:
    display.renderHome(envSensor.getTemp(), envSensor.getHum(), sysTime.getFormattedDate(), sysTime.getFormattedTime());
    break;
  case MENU_SCREEN:
    display.renderMenu(menuOptions, 5, menuIndex);
    break;
  case LIGHT_VIEW:
  {
    uint16_t vals[10];
    for (int i = 0; i < 10; i++)
      vals[i] = spectralSensor.getChannelValue(i);
    display.renderLight(channelLabels, vals);
    break;
  }
  case SETTINGS_VIEW:
    if (settingsState == SETTINGS_MAIN)
    {
      display.renderMenu(settingsOptions, 4, settingsMenuIndex);
    }
    else if (settingsState == CALIB_DRY || settingsState == CALIB_WET)
    {
      long sum = 0;
      String prompt = (settingsState == CALIB_DRY) ? "Hold in AIR" : "Place in WATER";
      for (int i = 10; i >= 0; i--)
      {
        int val = analogRead(1);
        sum += val;
        display.renderCalibration(prompt, i, val);
        delay(1000);
      }
      int avg = sum / 11;
      if (settingsState == CALIB_DRY)
      {
        tempDryValue = avg;
        settingsState = CALIB_WET;
      }
      else
      {
        tempWetValue = avg;
        zones[0].saveCalibration(tempDryValue, tempWetValue);
        settingsState = SETTINGS_MAIN;
      }
    }
    else if (settingsState == WIFI_SSID || settingsState == WIFI_PASS)
    {
      display.renderTextInput(settingsState == WIFI_SSID ? "SSID" : "PASS", inputBuffer, charset[charIndex]);
    }
    break;
  }

  if (!manualPumpOn)
    zones[0].turnPumpOff();

  delay(500);
}
