#include "DisplayManager.h"

void DisplayManager::renderHome(float temp, float hum, String date, String time)
{
    _u8g2.clearBuffer();
    _u8g2.setFont(u8g2_font_ncenB10_tr);
    _u8g2.setCursor(0, 12);
    _u8g2.print("Temp: " + String(temp, 1) + " F");
    _u8g2.setCursor(0, 28);
    _u8g2.print("Hum:  " + String(hum, 1) + " %");
    _u8g2.setCursor(0, 44);
    _u8g2.print("Date: " + date);
    _u8g2.setCursor(0, 60);
    _u8g2.print("Time: " + time);
    _u8g2.sendBuffer();
}

void DisplayManager::renderLight(const char **labels, int *values)
{
    _u8g2.clearBuffer();
    _u8g2.setFont(u8g2_font_ncenB10_tr);
    _u8g2.drawStr(0, 10, "Light Spectrum");
    _u8g2.setCursor(0, 30);
    _u8g2.print("F6 (590nm): ");
    _u8g2.print(values[5]);
    _u8g2.sendBuffer();
}

void DisplayManager::renderWatering(String name, int percent, int raw)
{
    _u8g2.clearBuffer();
    _u8g2.setFont(u8g2_font_ncenB10_tr);
    _u8g2.setCursor(0, 15);
    _u8g2.print(name);
    _u8g2.setCursor(0, 40);
    _u8g2.print("Moisture: " + String(percent) + "%");
    _u8g2.setFont(u8g2_font_6x10_tr);
    _u8g2.setCursor(0, 60);
    _u8g2.print("Raw: " + String(raw));
    _u8g2.sendBuffer();
}

void DisplayManager::renderMenu(const char **options, int count, int selected)
{
    _u8g2.clearBuffer();
    _u8g2.setFont(u8g2_font_6x10_tr);
    for (int i = 0; i < count; i++)
    {
        _u8g2.setCursor(0, 12 + (i * 12));
        if (i == selected)
            _u8g2.print("> ");
        else
            _u8g2.print("  ");
        _u8g2.print(options[i]);
    }
    _u8g2.sendBuffer();
}