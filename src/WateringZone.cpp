#include "WateringZone.h"

// Initialize hardware pins and names
WateringZone::WateringZone(uint8_t sensorPin, uint8_t mcpPin, uint8_t id, String name)
    : _sensorPin(sensorPin), _mcpPin(mcpPin), _id(id), _plantName(name), _mcp(nullptr)
{
    _dryValue = 700;   // Default 0% calibration
    _waterValue = 330; // Default 100% calibration
    _currentRaw = 0;
}

void WateringZone::begin()
{
    // Match ESP32-S3 to the 10-bit resolution from the manufacturer
    analogReadResolution(10);

    // Load saved settings from ESP32 memory
    String folder = "zone" + String(_id);
    _prefs.begin(folder.c_str(), false);
    _plantName = _prefs.getString("name", "Plant " + String(_id + 1));
    _dryValue = _prefs.getInt("dry", 700);
    _waterValue = _prefs.getInt("water", 330);
    _prefs.end();

    pinMode(_sensorPin, INPUT);
    if (_mcp != nullptr)
    {
        _mcp->pinMode(_mcpPin, OUTPUT);
        _mcp->digitalWrite(_mcpPin, LOW); // Off
    }
}

void WateringZone::update()
{
    _currentRaw = analogRead(_sensorPin);
}

int WateringZone::getMoisturePercent()
{
    // Map inverted values: High (700) is air/dry 0%, Low (330) is submerged in water 100%
    int moisturePercent = map(_currentRaw, _dryValue, _waterValue, 0, 100);
    return constrain(moisturePercent, 0, 100);
}

void WateringZone::saveCalibration(int dry, int water)
{
    _dryValue = dry;
    _waterValue = water;

    // Save soil moisture sensor calibrated values
    String namespaceName = "zone" + String(_id);
    _prefs.begin(namespaceName.c_str(), false);
    _prefs.putInt("dry", _dryValue);
    _prefs.putInt("water", _waterValue);
    _prefs.end();
}

void WateringZone::attachMCP(Adafruit_MCP23X17 *mcp)
{
    _mcp = mcp;
    Serial.println("MCP attached to zone");
}

void WateringZone::turnPumpOn()
{
    if (_mcp == nullptr)
    {
        Serial.println("ERROR: MCP not attached!");
        return;
    }
    // Print when pump on and pin # in serial monitor
    Serial.print("Pump ON (pin ");
    Serial.print(_mcpPin);
    Serial.println(")");

    _mcp->digitalWrite(_mcpPin, HIGH);
}

void WateringZone::turnPumpOff()
{
    if (_mcp != nullptr)
        _mcp->digitalWrite(_mcpPin, LOW);
}

void WateringZone::rename(String newName)
{
    _plantName = newName;
    String folder = "zone" + String(_id);
    _prefs.begin(folder.c_str(), false);
    _prefs.putString("name", _plantName);
    _prefs.end();
}