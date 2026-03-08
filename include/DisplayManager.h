#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <U8g2lib.h>

enum Screen
{
    HOME,
    LIGHT,
    WATERING
};

class DisplayManager
{
private:
    U8G2_SSD1315_128X64_NONAME_F_HW_I2C &_u8g2;
    Screen _currentScreen;

public:
    DisplayManager(U8G2_SSD1315_128X64_NONAME_F_HW_I2C &u8g2) : _u8g2(u8g2), _currentScreen(HOME) {}
    void setScreen(Screen s) { _currentScreen = s; }
    void nextScreen();
    void renderHome(float temp, float hum, String date, String time);
    void renderLight(const char **labels, int *values);
    void renderWatering(String name, int percent, int raw);
    void renderMenu(const char **options, int count, int selected);
};

#endif