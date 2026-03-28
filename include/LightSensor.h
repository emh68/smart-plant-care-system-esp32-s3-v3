#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include <Adafruit_AS7341.h>

class LightSensor
{

private:
    Adafruit_AS7341 _as7341;
    uint16_t _readings[10];
    as7341_gain_t _currentGain;
    const uint8_t _spectrumChannels[8] = {
        AS7341_CHANNEL_415nm_F1, AS7341_CHANNEL_445nm_F2,
        AS7341_CHANNEL_480nm_F3, AS7341_CHANNEL_515nm_F4,
        AS7341_CHANNEL_555nm_F5, AS7341_CHANNEL_590nm_F6,
        AS7341_CHANNEL_630nm_F7, AS7341_CHANNEL_680nm_F8};

public:
    bool begin();
    void update();
    uint16_t getChannelValue(int index);
};

#endif