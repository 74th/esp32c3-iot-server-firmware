#include <Adafruit_NeoPixel.h>

#include <config.hpp>
#include <led.hpp>

Adafruit_NeoPixel pixels(1, LED_PIN, NEO_GRB + NEO_KHZ800);

void ledSetup(void)
{
    pixels.begin();
    pixels.setPixelColor(0, LED_COLOR_SETUP);
    pixels.show();
}

void ledShowColor(uint32_t color)
{
    pixels.setPixelColor(0, color);
    pixels.show();
}