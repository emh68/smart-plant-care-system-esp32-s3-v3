#include "DisplayManager.h"

void DisplayManager::renderHome(float temp, float hum, String date, String time)
{
    _tft->fillScreen(ST77XX_BLUE);
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setTextSize(2);

    _tft->setCursor(10, 10);
    _tft->print("Temp: " + String(temp, 1) + " F");

    _tft->setCursor(10, 30);
    _tft->print("Hum: " + String(hum, 1) + " %");

    _tft->setCursor(10, 60);
    _tft->print("Date: " + date);

    _tft->setCursor(10, 90);
    _tft->print("Time: " + time);
}

void DisplayManager::renderMenu(const char **options, int count, int selected)
{
    _tft->fillScreen(ST77XX_BLUE);
    _tft->setTextSize(2);

    int maxVisible = 5; // Max number of items that fit on screen
    int startItem = 0;

    // Scrolling Logic: If selected item moves off the bottom of the
    // screen, shift the "window" down so the selected item is at the bottom.
    if (selected >= maxVisible)
    {
        startItem = selected - (maxVisible - 1);
    }

    for (int i = 0; i < maxVisible && (startItem + i) < count; i++)
    {
        int currentIndex = startItem + i;
        int y = 20 + i * 40; // spacing

        _tft->setCursor(10, y);
        if (currentIndex == selected)
        {
            _tft->setTextColor(ST77XX_YELLOW);
            _tft->print("> ");
        }
        else
        {
            _tft->setTextColor(ST77XX_WHITE);
            _tft->print("  ");
        }
        _tft->print(options[currentIndex]);
    }
}

void DisplayManager::renderWatering(String name, int percent, int raw)
{
    _tft->fillScreen(ST77XX_BLACK);
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setTextSize(2);

    _tft->setCursor(10, 10);
    _tft->print(name);

    _tft->setCursor(10, 50);
    _tft->print("Moisture: " + String(percent) + "%");

    _tft->setTextSize(1);
    _tft->setCursor(10, 90);
    _tft->print("Raw: " + String(raw));

    // Draw soil moisture bar
    int barWidth = map(percent, 0, 100, 0, 200);
    _tft->fillRect(10, 110, barWidth, 20, ST77XX_BLUE);
    _tft->drawRect(10, 110, 200, 20, ST77XX_WHITE);
}

void DisplayManager::renderLight(const char **labels, uint16_t *values)
{
    _tft->fillScreen(ST77XX_BLACK);
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setTextSize(2);
    _tft->setCursor(10, 0);
    _tft->print("Light Spectrum");

    // Define RGB565 Colors (5-bits Red, 6-bits Green, 5-bits Blue) for the 10 channels
    uint16_t channelColors[] = {
        0x701D, // F1 415nm (Purple/Violet)
        0x015F, // F2 445nm (Deep Blue)
        0x06BF, // F3 480nm (Cyan/Blue)
        0x1FE0, // F4 515nm (Green)
        0xB7E0, // F5 555nm (Yellow-Green)
        0xFFE0, // F6 590nm (Yellow)
        0xFA60, // F7 630nm (Orange)
        0xF800, // F8 680nm (Red)
        0xFFFF, // Clear (White)
        0x4000  // NIR 910nm (Dark Maroon)
    };

    // X Axis labels (light wavelengths)
    const char *shortLabels[] = {"415", "445", "480", "515", "555", "590", "630", "680", "CLR", "NIR"};

    for (int i = 0; i < 10; i++)
    {
        // Map light level raw values 0-65535 to 180 pixels height
        int barHeight = map(values[i], 0, 65535, 0, 180);

        int xPos = 5 + (i * 31); // Bar column width 31px
        _tft->fillRect(xPos, 215 - barHeight, 22, barHeight, channelColors[i]);
        _tft->drawRect(xPos, 215 - 180, 22, 180, 0x4208);

        _tft->setTextSize(1);
        _tft->setCursor(xPos, 225);
        _tft->print(shortLabels[i]);
    }
}

// For entering or changing info. (WIFI SSID, password, plant names, etc.)
void DisplayManager::renderTextInput(String title, String currentInput, char activeChar)
{
    _tft->fillScreen(ST77XX_BLACK);
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setTextSize(2);
    _tft->setCursor(10, 10);
    _tft->print(title);

    // Draw characters currently entered
    _tft->setCursor(10, 50);
    _tft->setTextSize(3);
    _tft->setTextColor(ST77XX_CYAN);
    _tft->print(currentInput);

    // Draw character user is currently hovering over
    _tft->setTextColor(ST77XX_YELLOW);
    _tft->print(activeChar);

    _tft->setTextSize(1);
    _tft->setCursor(10, 100);
    _tft->setTextColor(ST77XX_WHITE);
    _tft->print("Scroll to change, Click to select");
    _tft->setCursor(10, 120);
    _tft->print("Use '>' for Enter, '<' for Backspace");
}

void DisplayManager::renderCalibration(String instruction, int countdown, int currentRaw)
{
    _tft->fillScreen(ST77XX_BLACK);
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setTextSize(2);
    _tft->setCursor(10, 20);
    _tft->print(instruction);

    _tft->setTextSize(6);
    _tft->setCursor(110, 100);
    _tft->setTextColor(ST77XX_YELLOW);
    _tft->print(countdown);

    _tft->setTextSize(2);
    _tft->setTextColor(ST77XX_GREEN);
    _tft->setCursor(10, 200);
    _tft->print("Current Raw: " + String(currentRaw));
}

void DisplayManager::renderPlantProfile(
    String plantName,
    int moistureMin,
    int moistureMax,
    int humidityMin,
    int humidityMax,
    int tempMin,
    int tempMax,
    float soilPHMin,
    float soilPHMax,
    String lightNeeds,
    int wateringFrequencyDays)
{
    _tft->fillScreen(ST77XX_BLACK);
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setCursor(0, 0);

    _tft->println("Plant Profile");
    _tft->println("----------------");
    _tft->println(plantName);
    _tft->println("");

    _tft->print("Moisture: ");
    _tft->print(moistureMin);
    _tft->print("-");
    _tft->println(moistureMax);

    _tft->print("Humidity: ");
    _tft->print(humidityMin);
    _tft->print("-");
    _tft->println(humidityMax);

    _tft->print("Temp: ");
    _tft->print(tempMin);
    _tft->print("-");
    _tft->println(tempMax);

    _tft->print("Soil pH: ");
    _tft->print(soilPHMin, 1);
    _tft->print("-");
    _tft->println(soilPHMax, 1);

    _tft->print("Light: ");
    _tft->println(lightNeeds);

    _tft->print("Water every: ");
    _tft->print(wateringFrequencyDays);
    _tft->println(" days");

    _tft->println("");
    _tft->println("Press button to go back");
}