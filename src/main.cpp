#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "Seeed_SHT35.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <time.h>
#include "PCF85063TP.h"
#include "secrets.h"

// --------------------
// Hardware / Libraries
// --------------------

// Grove SHT35
SHT35 sensor(6);

// Grove OLED SSD1315: use the SSD1315 constructor and set address explicitly
U8G2_SSD1315_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
// U8G2_SSD1315_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/6, /* data=*/5, /* reset=*/U8X8_PIN_NONE);
static constexpr uint8_t OLED_ADDR_7BIT = 0x3C;

// Grove High Precision RTC v1.0 (PCF85063)
PCF85063TP RTclock;
static constexpr uint8_t RTC_ADDR_7BIT = 0x51;

// NTP
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 0, 3600000);

// --------------------
// Time configuration
// --------------------
int tzOffsetHours = -7; // Mountain Time standard offset
bool dstActive = true;  // add 1 hour when DST active

// --------------------
// Stored last-known-good time
// --------------------
int lastDay = 0, lastMonth = 0, lastYear = 0;
int lastHour = 0, lastMinute = 0, lastSecond = 0;

bool rtcPresent = false;      // detected on I2C
bool rtcTrusted = false;      // reads produce sane values
bool systemTimeValid = false; // NTP (or RTC->system) set system clock at least once

// --------------------
// Helpers
// --------------------
bool i2cDevicePresent(uint8_t addr7)
{
  Wire.beginTransmission(addr7);
  return (Wire.endTransmission() == 0);
}

bool timeFieldsSane(int year, int month, int day, int hour, int minute, int second)
{
  if (year < 2020 || year > 2099)
    return false;
  if (month < 1 || month > 12)
    return false;
  if (day < 1 || day > 31)
    return false;
  if (hour < 0 || hour > 23)
    return false;
  if (minute < 0 || minute > 59)
    return false;
  if (second < 0 || second > 59)
    return false;
  return true;
}

bool updateDisplayedTimeFromSystem()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 1000))
    return false;

  const int y = timeinfo.tm_year + 1900;
  const int mo = timeinfo.tm_mon + 1;
  const int d = timeinfo.tm_mday;
  const int h = timeinfo.tm_hour;
  const int mi = timeinfo.tm_min;
  const int s = timeinfo.tm_sec;

  if (!timeFieldsSane(y, mo, d, h, mi, s))
    return false;

  lastYear = y;
  lastMonth = mo;
  lastDay = d;
  lastHour = h;
  lastMinute = mi;
  lastSecond = s;
  return true;
}

bool updateDisplayedTimeFromRTC()
{
  if (!rtcPresent)
    return false;

  RTclock.getTime();

  const int y = RTclock.year + 2000;
  const int mo = RTclock.month;
  const int d = RTclock.dayOfMonth;
  const int h = RTclock.hour;
  const int mi = RTclock.minute;
  const int s = RTclock.second;

  if (!timeFieldsSane(y, mo, d, h, mi, s))
    return false;

  lastYear = y;
  lastMonth = mo;
  lastDay = d;
  lastHour = h;
  lastMinute = mi;
  lastSecond = s;
  return true;
}

void syncRTCwithSystemTime()
{
  // Only call when getLocalTime() succeeds
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 2000))
    return;

  RTclock.stopClock();
  RTclock.fillByYMD(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
  RTclock.fillByHMS(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  RTclock.fillDayOfWeek(timeinfo.tm_wday);
  RTclock.setTime();
  RTclock.startClock();
}

bool connectWiFiWithTimeout(int maxAttempts, int delayMs)
{
  WiFi.begin(ssid, password);
  for (int i = 0; i < maxAttempts; i++)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      delay(1000);
      return true;
    }
    delay(delayMs);
  }
  return (WiFi.status() == WL_CONNECTED);
}

void disconnectWiFi()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// --------------------
// Setup
// --------------------
void setup()
{
  Serial.begin(115200);
  delay(2000);

  Wire.begin(5, 6);
  Wire.setClock(100000);
  delay(200);

  u8g2.setI2CAddress(OLED_ADDR_7BIT << 1);
  u8g2.begin();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(0, 30, "Booting...");
  u8g2.sendBuffer();

  // Sensor init
  if (sensor.init())
    Serial.println("SHT35 init failed!!!");
  else
    Serial.println("SHT35 Online.");

  // Detect RTC presence before using it
  rtcPresent = i2cDevicePresent(RTC_ADDR_7BIT);
  if (rtcPresent)
  {
    RTclock.begin();
    rtcTrusted = updateDisplayedTimeFromRTC();
    Serial.println(rtcTrusted ? "RTC detected and readable." : "RTC detected but not trusted.");
  }
  else
  {
    rtcTrusted = false;
    Serial.println("RTC not detected (continuing without it).");
  }

  // Try to get system time from NTP at boot if Wi-Fi is available
  if (connectWiFiWithTimeout(20, 500))
  {
    long tzOffsetSeconds = (long)tzOffsetHours * 3600L;
    long dstOffsetSeconds = dstActive ? 3600L : 0L;
    configTime(tzOffsetSeconds, dstOffsetSeconds, "pool.ntp.org", "time.nist.gov");

    // Wait for time to become available
    if (updateDisplayedTimeFromSystem())
    {
      systemTimeValid = true;
      Serial.println("System time set from NTP.");

      // If RTC exists, sync it to NTP/system time
      if (rtcPresent)
      {
        syncRTCwithSystemTime();
        rtcTrusted = true;
        Serial.println("RTC synced from NTP/system time.");
      }
    }
    else
    {
      Serial.println("NTP did not provide time (boot).");
      systemTimeValid = false;
    }

    disconnectWiFi();
  }
  else
  {
    Serial.println("WiFi not connected at boot.");
    systemTimeValid = false;

    // If no NTP, fall back to RTC if it is good
    if (rtcTrusted)
      Serial.println("Using RTC time.");
    else
      Serial.println("No valid time source yet (will show last-known/blank time fields).");
  }
}

// --------------------
// Loop
// --------------------
void loop()
{
  // Prefer system time if valid (NTP-set clock continues even with Wi-Fi off)
  bool timeOk = false;

  if (systemTimeValid)
    timeOk = updateDisplayedTimeFromSystem();

  // If system time isn't valid yet, try RTC (if present)
  if (!timeOk && rtcTrusted)
    timeOk = updateDisplayedTimeFromRTC();

  // Hourly resync attempt (only if Wi-Fi available)
  static unsigned long lastSyncMs = 0;
  if (millis() - lastSyncMs > 3600000UL)
  {
    if (connectWiFiWithTimeout(15, 500))
    {
      long tzOffsetSeconds = (long)tzOffsetHours * 3600L;
      long dstOffsetSeconds = dstActive ? 3600L : 0L;
      configTime(tzOffsetSeconds, dstOffsetSeconds, "pool.ntp.org", "time.nist.gov");

      if (updateDisplayedTimeFromSystem())
      {
        systemTimeValid = true;
        if (rtcPresent)
        {
          syncRTCwithSystemTime();
          rtcTrusted = true;
        }
        Serial.println("Hourly NTP sync OK.");
      }
      else
      {
        Serial.println("Hourly NTP sync failed.");
      }

      disconnectWiFi();
    }
    lastSyncMs = millis();
  }

  // --------------------
  // Display
  // --------------------
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);

  float temp, hum;
  if (NO_ERROR != sensor.read_meas_data_single_shot(HIGH_REP_WITH_STRCH, &temp, &hum))
  {
    u8g2.setCursor(0, 12);
    u8g2.print("Read temp failed!!");
  }
  else
  {
    u8g2.setCursor(0, 12);
    u8g2.print("Temp: ");

    int tempX = u8g2.getCursorX();
    u8g2.print((temp * 1.8) + 32);

    int afterNumX = u8g2.getCursorX();

    u8g2.print(" ");
    afterNumX = u8g2.getCursorX();

    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.drawGlyph(afterNumX, 10, 0x00b0);
    // u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.setFont(u8g2_font_helvB10_tr);
    u8g2.print(" F");
    u8g2.setCursor(0, 27);
    u8g2.print("Hum: ");
    u8g2.print(hum);
    u8g2.print(" %");
    u8g2.setCursor(0, 42);
    u8g2.print("Date: ");
    if (timeOk || systemTimeValid || rtcTrusted)
    {
      u8g2.print(lastMonth);
      u8g2.print("/");
      u8g2.print(lastDay);
      u8g2.print("/");
      u8g2.print(lastYear);
    }
    else
    {
      u8g2.print("--/--/----");
    }

    u8g2.setCursor(0, 57);
    u8g2.print("Time: ");
    if (timeOk || systemTimeValid || rtcTrusted)
    {
      char formattedTime[12];
      int displayHour = lastHour % 12;
      if (displayHour == 0)
        displayHour = 12;
      const char *period = (lastHour >= 12) ? "PM" : "AM";
      sprintf(formattedTime, "%02d:%02d:%02d %s", displayHour, lastMinute, lastSecond, period);
      u8g2.print(formattedTime);
    }
    else
    {
      u8g2.print("--:--:--");
    }
  }

  u8g2.sendBuffer();
  delay(1000);
}
