#include "TempHumSensor.h"

TempHumSensor::TempHumSensor(int pinNum) : _sht35(pinNum)
{
    _currentTemp = 0.0;
    _currentHum = 0.0;
}
bool TempHumSensor::init()
{
    return (_sht35.init() == 0);
}

void TempHumSensor::update()
{
    float t = 0;
    float h = 0;

    // Retrieve data (temp + hum) pass variables by reference to return temp & hum as two separate values
    if (_sht35.read_meas_data_single_shot(HIGH_REP_WITH_STRCH, &t, &h) == NO_ERROR)
    {
        _currentTemp = (t * 1.8) + 32; // Convert Celsius to Fahrenheit
        _currentHum = h;
    }
}

float TempHumSensor::getTemp()
{
    return _currentTemp;
}

float TempHumSensor::getHum()
{
    return _currentHum;
}