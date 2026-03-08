#include "SystemTime.h"

void SystemTime::begin()
{
    _rtcPresent = true;
    _rtc.begin();
}

bool SystemTime::syncNTP()
{
    static bool clientStarted = false;
    if (!clientStarted)
    {
        _timeClient.begin();
        clientStarted = true;
    }
    if (_timeClient.update())
    {
        unsigned long epochTime = _timeClient.getEpochTime();
        struct tm *ptm = gmtime((time_t *)&epochTime);

        _rtc.stopClock();
        // time.h library counts years since 1900 and months from 0-11, so tm_year and tm_month are offset
        _rtc.fillByYMD(ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
        _rtc.fillByHMS(ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
        _rtc.setTime();
        _rtc.startClock();
        return true;
    }
    return false;
}

String SystemTime::getFormattedTime()
{
    _rtc.getTime();
    char buf[12];
    int h = _rtc.hour % 12;
    if (h == 0)
        h = 12;
    sprintf(buf, "%02d:%02d %s", h, _rtc.minute, (_rtc.hour >= 12 ? "PM" : "AM"));
    return String(buf);
}

String SystemTime::getFormattedDate()
{
    _rtc.getTime();
    return String(_rtc.month) + "/" + String(_rtc.dayOfMonth) + "/" + String(_rtc.year + 2000);
}