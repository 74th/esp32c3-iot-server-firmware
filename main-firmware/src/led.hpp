#include <Adafruit_NeoPixel.h>

#define LED_COLOR_SETUP Adafruit_NeoPixel::Color(0, 32, 0)
#define LED_COLOR_NORMAL Adafruit_NeoPixel::Color(0, 0, 32)
#define LED_COLOR_RECEIVE Adafruit_NeoPixel::Color(32, 32, 0)
#define LED_COLOR_OPERATION Adafruit_NeoPixel::Color(0, 32, 32)

void ledSetup(void);
void ledShowColor(uint32_t color);