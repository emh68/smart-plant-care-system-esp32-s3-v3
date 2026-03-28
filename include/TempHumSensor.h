#ifndef TEMP_HUM_SENSOR_H
#define TEMP_HUM_SENSOR_H

#include "Seeed_SHT35.h"

class TempHumSensor
{
private:
    SHT35 _sht35;
    // Temperature & Humidity are separated (sensor reads them as one)
    float _currentTemp; // Stored in Fahrenheit
    float _currentHum;  // Stored in % Relative Humidity

public:
    TempHumSensor(int pinNum);
    bool init();
    void update();
    float getTemp();
    float getHum();
};

#endif