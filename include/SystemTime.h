#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "PCF85063TP.h"

class SystemTime
{
private:
    PCF85063TP _rtc;
    WiFiUDP _udp;
    NTPClient _timeClient;
    int _offset;
    bool _rtcPresent;

public:
    SystemTime(int offsetHours) : _timeClient(_udp, "pool.ntp.org"), _offset(offsetHours * 3600) {}
    void begin();
    bool syncNTP();
    void updateFromRTC();
    String getFormattedTime();
    String getFormattedDate();
    bool isReady();
};

#endif