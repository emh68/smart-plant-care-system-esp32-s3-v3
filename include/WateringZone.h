#ifndef WATERING_ZONE_H
#define WATERING_ZONE_H

#include <Arduino.h>
#include <Preferences.h>
#include <Adafruit_MCP23X17.h>

class WateringZone
{
private:
    uint8_t _sensorPin;  // ADS1115 (analog channels A0, A1, ...)
    uint8_t _mcpPin;     // MCP23017 solenoid driver
    uint8_t _id;         // Unique ID for scaling multiple plants/sensors/pumps
    int _dryValue;       // For reading dry/air calibration point
    int _waterValue;     // For reading fully wet/water calibration point
    int _currentRaw;     // Actual raw data value
    int _triggerPercent; // Moisture percent to trigger watering
    int _targetPercent;  // Target moisture percent
    String _plantName;
    Preferences _prefs;      // Each plant gets its own memory for calibration values
    Adafruit_MCP23X17 *_mcp; // pointer to shared MCP23017 solenoid driver

public:
    // Pairs sensor with pump for a specific plant
    WateringZone(uint8_t sensorPin, uint8_t mcpPin, uint8_t id, String name);

    void attachMCP(Adafruit_MCP23X17 *mcp);
    void setCalibration(int dry, int wet)
    {
        _dryValue = dry;
        _waterValue = wet;
    }

    void begin();                             // Load saved values from memory
    void saveCalibration(int dry, int water); // Save calibration values for power loss
    void setThresholds(int trigger, int target);
    String getPlantName() { return _plantName; }
    int getCurrentRaw() { return _currentRaw; }
    int getTarget() { return _targetPercent; }
    int getTrigger() { return _triggerPercent; }

    void update();
    int getMoisturePercent(); // Logic for 0-100% mapping
    void turnPumpOn();
    void turnPumpOff();
    void rename(String newName);
    uint8_t getId() { return _id; }
};

#endif