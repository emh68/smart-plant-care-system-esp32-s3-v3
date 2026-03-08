#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "Seeed_SHT35.h"
#include <WiFi.h>
#include "secrets.h"
#include "LightSensor.h"
#include "TempHumSensor.h"
#include "WateringZone.h"
#include "SystemTime.h"
#include "DisplayManager.h"

U8G2_SSD1315_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

TempHumSensor envSensor(6);
LightSensor spectralSensor;
SystemTime sysTime(-7); // Mountain Time Zone (user change feature next iteration)
DisplayManager display(u8g2);

const char *channelLabels[] = {"F1-415nm", "F2-445nm", "F3-480nm", "F4-515nm", "F5-555nm", "F6-590nm", "F7-630nm", "F8-680nm", "Clear", "NIR"};
WateringZone zones[] = {WateringZone(3, 9, 0, "Plant 1")};

// Navigation States
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
  u8g2.begin();
  sysTime.begin();
  envSensor.init();
  spectralSensor.begin();
  zones[0].begin();
}

void loop()
{
  envSensor.update();
  spectralSensor.update();
  zones[0].update();

  // Light Wavelength levels Serial Output (for Plotter and debug checks)
  for (int i = 0; i < 10; i++)
  {
    Serial.print(">");
    Serial.print(channelLabels[i]);
    Serial.print(":");
    Serial.println(spectralSensor.getChannelValue(i));
  }

  // Soil Moisture Serial Debug Output
  Serial.print("Plant: ");
  Serial.print(zones[0].getId());
  Serial.print(" | Raw: ");
  Serial.print(analogRead(3));
  Serial.print(" | Moisture: ");
  Serial.print(zones[0].getMoisturePercent());
  Serial.println("%");
  Serial.println();

  // Read Rotary Encoder Button Depress
  Wire.beginTransmission(ENCODER_ADDR);
  Wire.write(REG_BUTTON);
  Wire.endTransmission();
  Wire.requestFrom(ENCODER_ADDR, (uint8_t)1);

  if (Wire.available())
  {
    uint8_t currentBtnStatus = Wire.read();
    if (currentBtnStatus == 1 && lastBtnStatus == 0)
    {
      if (currentState == DASHBOARD)
      {
        currentState = MENU_SCREEN;
      }
      else if (currentState == MENU_SCREEN)
      {
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
            zones[0].setPumpSpeed(255);
            menuOptions[3] = "Pump: ON";
          }
          else
          {
            zones[0].setPumpSpeed(0);
            menuOptions[3] = "Pump: OFF";
          }
        }
        else if (menuIndex == 4)
          currentState = SETTINGS_VIEW;
      }
      else
      {
        currentState = MENU_SCREEN;
      }
      delay(200);
    }
    lastBtnStatus = currentBtnStatus;
  }

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
      if (currentState == MENU_SCREEN)
      {
        if (currentPos > lastEncoderPos)
          menuIndex = (menuIndex + 1) % 5;
        else
          menuIndex = (menuIndex - 1 + 5) % 5;
      }
      lastEncoderPos = currentPos;
    }
  }

  // Render Logic
  switch (currentState)
  {
  case DASHBOARD:
    display.renderHome(envSensor.getTemp(), envSensor.getHum(), sysTime.getFormattedDate(), sysTime.getFormattedTime());
    break;
  case MENU_SCREEN:
    display.renderMenu(menuOptions, 5, menuIndex);
    break;
  case PLANT_VIEW:
    display.renderWatering(String(zones[0].getId()), zones[0].getMoisturePercent(), analogRead(3));
    break;
  case LIGHT_VIEW:
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(0, 20, "Light Levels");
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 40, "Active Plotting...");
    u8g2.sendBuffer();
    break;
  case SETTINGS_VIEW:
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(0, 20, "Settings");
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 40, "WiFi/Rename/Calib");
    u8g2.sendBuffer();
    break;
  }

  if (!manualPumpOn)
  {
    zones[0].setPumpSpeed(0);
  }

  delay(20);
}