#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

enum Screen
{
    HOME,
    LIGHT,
    WATERING
};

class DisplayManager
{
private:
    Adafruit_ST7789 *_tft;
    Screen _currentScreen;

public:
    DisplayManager(Adafruit_ST7789 *tft) : _tft(tft), _currentScreen(HOME) {}
    void setScreen(Screen s) { _currentScreen = s; }
    void nextScreen();
    // Functions to render different screens
    void renderHome(float temp, float hum, String date, String time);
    void renderLight(const char **labels, uint16_t *values);
    void renderWatering(String name, int percent, int raw);
    void renderMenu(const char **options, int count, int selected);
    void renderTextInput(String title, String currentInput, char activeChar);
    void renderCalibration(String instruction, int countdown, int currentRaw);

    void clearScreen() { _tft->fillScreen(ST77XX_BLACK); }
};

#endif