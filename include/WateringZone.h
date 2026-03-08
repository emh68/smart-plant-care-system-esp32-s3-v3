#ifndef WATERING_ZONE_H
#define WATERING_ZONE_H

#include <Arduino.h>
#include <Preferences.h>

class WateringZone
{
private:
    uint8_t _sensorPin;
    uint8_t _pumpPin;
    uint8_t _id;     // Unique ID for scaling multiple plants/sensors/pumps
    int _dryValue;   // Values 520-430 (0% dry soil)
    int _waterValue; // Values 350-260 (100% water)
    int _currentRaw; // Actual raw data value
    int _triggerLevel;
    int _targetLevel;
    String _plantName;
    Preferences _prefs; // Each plant gets its own memory for calibration values

public:
    // Pairs sensor with pump for a specific plant
    WateringZone(uint8_t sensorPin, uint8_t pumpPin, uint8_t id, String name);

    void begin();                             // Load saved values from memory
    void saveCalibration(int dry, int water); // Save calibration values for power loss
    void setThresholds(int trigger, int target);
    int getTrigger() { return _triggerLevel; }

    void update();
    int getMoisturePercent();     // Logic for 0-100% mapping
    void setPumpSpeed(int speed); // 0-255 PWM for MOSFET control
    void rename(String newName);
    uint8_t getId() { return _id; }
};

#endif