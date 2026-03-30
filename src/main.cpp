/*
 * Smart Plant Care System v3.0 - Main Application
 * Integrated with Firebase RTDB for Cloud-Synced plant library and sensor data logging
 * Features: Real-time sensor monitoring/telemetry (SHT35 - temp/hum, AS7341 - light intensity different wavelengths, Soil moisture)
 * Sync with cloud database which includes 13 plant dynamic library for sensor comparison/baseline
 * Sensor logging to cloud database for sensor history
 * CRUD operations
 */

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
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Variables for the on-screen keyboard
const String charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
int charIndex = 0;
String inputBuffer = "";

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool firebaseReady = false;
unsigned long sendDataPrevMillis = 0;

// Plant library storage
const int MAX_PLANTS = 20;
String plantIds[MAX_PLANTS];
String plantNames[MAX_PLANTS];
int plantCount = 0;
int selectedPlantIndex = 0;

// Variables for active plant profile fetched from cloud database
String selectedPlantId = "";
String selectedPlantName = "";

int selectedMoistureMin = 0;
int selectedMoistureMax = 0;
int selectedHumidityMin = 0;
int selectedHumidityMax = 0;
int selectedTempMin = 0;
int selectedTempMax = 0;
float selectedSoilPHMin = 0.0;
float selectedSoilPHMax = 0.0;
String selectedLightNeeds = "";
int selectedWateringFrequencyDays = 0;

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
  SETTINGS_WIFI_SSID,
  SETTINGS_WIFI_PASS,
  SETTINGS_CALIB_DRY,
  SETTINGS_CALIB_WET,
  SETTINGS_RENAME_PLANT
};
SettingsSubState settingsState = SETTINGS_MAIN;
const char *settingsOptions[] = {"[Back]", "WiFi Config", "Calibrate Sensor", "Clear DB Logs"};
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
SystemTime sysTime(-6); // Mountain Time Zone
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
  PLANT_LIBRARY_VIEW,
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

const char *menuOptions[] = {"[Exit Menu]", "Plant Library", "Light Levels", "Pump: OFF", "Settings"};

void initFirebase()
{
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(16384);

  auth.user.email = "";
  auth.user.password = "";

  config.signer.test_mode = true;
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);

  Serial.println("Waiting for Firebase...");
  unsigned long start = millis();
  while (!Firebase.ready() && millis() - start < 10000)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (Firebase.ready())
  {
    Serial.println("Firebase READY");
    firebaseReady = true;
  }
  else
  {
    Serial.println("Firebase NOT READY");
    firebaseReady = false;
  }
}

void loadPlantLibrary()
{
  if (!firebaseReady)
    return;

  plantCount = 0;
  String tempIds[MAX_PLANTS];
  int idsFound = 0;

  Serial.println("--- Starting Safe Load (Small Packets) ---");

  // Fetch the index array (Small packet)
  if (Firebase.RTDB.getArray(&fbdo, "/plantIndex"))
  {
    FirebaseJsonArray &arr = fbdo.jsonArray();
    idsFound = (arr.size() > MAX_PLANTS) ? MAX_PLANTS : arr.size();

    for (size_t i = 0; i < idsFound; i++)
    {
      FirebaseJsonData res;
      arr.get(res, i);
      tempIds[i] = res.to<String>();
      tempIds[i].replace("\"", "");
      tempIds[i].trim();
    }
  }
  else
  {
    Serial.print("Index Load Failed: ");
    Serial.println(fbdo.errorReason());
    return;
  }

  // Fetch each name individually (Small packets)
  for (int i = 0; i < idsFound; i++)
  {
    if (tempIds[i] == "" || tempIds[i] == "null")
      continue;

    String namePath = "/plantLibrary/";
    namePath.concat(tempIds[i]);
    namePath.concat("/plantName");

    // This call is small so SSL will handle this easily
    if (Firebase.RTDB.getString(&fbdo, namePath.c_str()))
    {
      plantIds[plantCount] = tempIds[i];
      plantNames[plantCount] = fbdo.stringData();

      Serial.printf("Loaded [%d]: %s\n", plantCount, plantNames[plantCount].c_str());
      plantCount++;
    }
    delay(20);
  }

  Serial.print("Final Count: ");
  Serial.println(plantCount);
}

void applyPlantProfile(String plantId)
{
  if (!firebaseReady)
    return;

  selectedPlantId = plantId;
  String base = "/plantLibrary/";
  base.concat(plantId);

  // Use the small-packet fetch
  if (!Firebase.RTDB.getJSON(&fbdo, base.c_str()))
    return;

  FirebaseJson &json = fbdo.jsonObject();
  FirebaseJsonData result;
  // Specific environmental targets from cloud JSON
  if (json.get(result, "plantName"))
    selectedPlantName = result.to<String>();
  if (json.get(result, "moistureMin"))
    selectedMoistureMin = result.to<int>();
  if (json.get(result, "moistureMax"))
    selectedMoistureMax = result.to<int>();
  if (json.get(result, "humidityMin"))
    selectedHumidityMin = result.to<int>();
  if (json.get(result, "humidityMax"))
    selectedHumidityMax = result.to<int>();
  if (json.get(result, "tempMin"))
    selectedTempMin = result.to<int>();
  if (json.get(result, "tempMax"))
    selectedTempMax = result.to<int>();
  if (json.get(result, "soilPhMin"))
    selectedSoilPHMin = result.to<float>();
  if (json.get(result, "soilPhMax"))
    selectedSoilPHMax = result.to<float>();
  if (json.get(result, "lightNeeds"))
    selectedLightNeeds = result.to<String>();
  if (json.get(result, "wateringFrequencyDays"))
    selectedWateringFrequencyDays = result.to<int>();

  // Syncs hardware watering zone with data from cloud database
  zones[0].rename(selectedPlantName);
  zones[0].setThresholds(selectedMoistureMin, selectedMoistureMax);
}

void logSensorData(bool pumpActivated, int pumpDurationMs)
{
  if (!firebaseReady)
    return;

  FirebaseJson json;
  json.set("plantId", (selectedPlantId != "") ? selectedPlantId : "unknown");
  json.set("plantName", zones[0].getPlantName());

  // Format Timestamp for cloud database
  String fullTime = sysTime.getFormattedDate();
  fullTime += " ";
  fullTime += sysTime.getFormattedTime();

  json.set("timestamp", fullTime);
  json.set("soilMoisture", zones[0].getMoisturePercent());
  json.set("temperature", envSensor.getTemp());
  json.set("humidity", envSensor.getHum());
  json.set("lightLevel", spectralSensor.getChannelValue(8));
  json.set("pumpActivated", pumpActivated);
  json.set("pumpDurationMs", pumpDurationMs);

  if (Firebase.RTDB.pushJSON(&fbdo, "/sensorData", &json))
  {
    Serial.println("Sensor data logged.");
  }
  else
  {
    Serial.println("Failed to log sensor data:");
    Serial.println(fbdo.errorReason());
    Serial.print("HTTP code: ");
    Serial.println(fbdo.httpCode());
  }
}

void clearSensorLogs()
{
  if (!firebaseReady)
    return;
  if (Firebase.RTDB.deleteNode(&fbdo, "/sensorData"))
  {
    Serial.println("All sensor logs DELETED successfully.");
  }
  else
  {
    Serial.print("Failed to delete logs: ");
    Serial.println(fbdo.errorReason());
  }
}

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

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");

  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20)
  {
    delay(500);
    Serial.print(".");
    wifiTimeout++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected!");
    Serial.println(WiFi.localIP());

    initFirebase();

    if (firebaseReady)
    {
      delay(2000); // give SSL stack time to settle
      loadPlantLibrary();
    }
  }
  else
  {
    Serial.println("\nWiFi failed.");
  }
}

void loop()
{
  sysTime.syncNTP(); // Sync RTC with network time
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
        currentState = PLANT_LIBRARY_VIEW;
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
          settingsState = SETTINGS_WIFI_SSID;
          inputBuffer = "";
          charIndex = 0;
        }
        else if (settingsMenuIndex == 2)
        {
          settingsState = SETTINGS_CALIB_DRY;
        }
        else if (settingsMenuIndex == 3)
        {
          clearSensorLogs();
          currentState = MENU_SCREEN;
        }
      }
      else if (settingsState == SETTINGS_WIFI_SSID || settingsState == SETTINGS_WIFI_PASS)
      {
        char currentLetter = charset[charIndex];
        if (currentLetter == '>') // Used as Enter
        {
          if (settingsState == SETTINGS_WIFI_SSID)
          {
            settingsState = SETTINGS_WIFI_PASS;
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
    else if (currentState == PLANT_LIBRARY_VIEW)
    {
      if (selectedPlantIndex == 0)
      {
        currentState = MENU_SCREEN;
      }
      else if (plantCount > 0)
      {
        int actualPlantIdx = selectedPlantIndex - 1;
        applyPlantProfile(plantIds[actualPlantIdx]);
        currentState = PLANT_VIEW;
      }
    }
    else if (currentState == PLANT_VIEW)
    {
      currentState = PLANT_LIBRARY_VIEW;
    }
    else if (currentState == LIGHT_VIEW)
    {
      currentState = MENU_SCREEN;
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
      else if (currentState == PLANT_LIBRARY_VIEW)
      {
        if (plantCount > 0)
        {
          selectedPlantIndex = forward ? (selectedPlantIndex + 1) % (plantCount + 1) : (selectedPlantIndex - 1 + (plantCount + 1)) % (plantCount + 1);
        }
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
  case PLANT_LIBRARY_VIEW:
  {
    if (plantCount == 0)
    {
      static const char *emptyOptions[] = {"No plants loaded", "[Back]"};
      display.renderMenu(emptyOptions, 2, selectedPlantIndex);
    }
    else
    {
      const char *plantMenu[MAX_PLANTS + 1];
      static String menuItems[MAX_PLANTS + 1];

      menuItems[0] = "[Back]";
      plantMenu[0] = menuItems[0].c_str();

      for (int i = 0; i < plantCount; i++)
      {
        menuItems[i + 1] = plantNames[i];
        plantMenu[i + 1] = menuItems[i + 1].c_str();
      }

      display.renderMenu(plantMenu, plantCount + 1, selectedPlantIndex);
    }
    break;
  }
  case PLANT_VIEW:
    display.renderPlantProfile(
        selectedPlantName,
        selectedMoistureMin,
        selectedMoistureMax,
        selectedHumidityMin,
        selectedHumidityMax,
        selectedTempMin,
        selectedTempMax,
        selectedSoilPHMin,
        selectedSoilPHMax,
        selectedLightNeeds,
        selectedWateringFrequencyDays);
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
    else if (settingsState == SETTINGS_CALIB_DRY || settingsState == SETTINGS_CALIB_WET)
    {
      long sum = 0;
      String prompt = (settingsState == SETTINGS_CALIB_DRY) ? "Hold in AIR" : "Place in WATER";
      for (int i = 10; i >= 0; i--)
      {
        int val = analogRead(1);
        sum += val;
        display.renderCalibration(prompt, i, val);
        delay(1000);
      }
      int avg = sum / 11;
      if (settingsState == SETTINGS_CALIB_DRY)
      {
        tempDryValue = avg;
        settingsState = SETTINGS_CALIB_WET;
      }
      else
      {
        tempWetValue = avg;
        zones[0].saveCalibration(tempDryValue, tempWetValue);
        settingsState = SETTINGS_MAIN;
        currentState = MENU_SCREEN;
      }
    }
    else if (settingsState == SETTINGS_WIFI_SSID || settingsState == SETTINGS_WIFI_PASS)
    {
      display.renderTextInput(settingsState == SETTINGS_WIFI_SSID ? "SSID" : "PASS", inputBuffer, charset[charIndex]);
    }
    break;
  }

  if (!manualPumpOn)
    zones[0].turnPumpOff();

  const unsigned long sensorWriteInterval = 60UL * 60UL * 1000UL;
  // const unsigned long sensorWriteInterval = 10UL * 60UL * 1000UL; // 10 minutes
  // const unsigned long sensorWriteInterval = 30UL * 1000UL; // 30 seconds

  // Pushes sensor data to cloud database only if time interval has passed
  if (firebaseReady && millis() - sendDataPrevMillis > sensorWriteInterval)
  {
    sendDataPrevMillis = millis();
    logSensorData(manualPumpOn, manualPumpOn ? 1000 : 0);
  }

  delay(500);
}
