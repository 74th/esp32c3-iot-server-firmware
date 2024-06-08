#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ssid.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN GPIO_NUM_2

const char *ssid = WIFI_SSID;
const char *pass = WIFI_PASSWORD;

WebServer server(80);
Adafruit_NeoPixel pixels(1, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
    Serial.begin(115200);

    Serial.println("Start Initialize");

    pixels.begin();
    pixels.setPixelColor(0, pixels.Color(0, 32, 0));
    pixels.show();

    // WiFiの接続
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }
    Serial.print("IP:");
    Serial.println(WiFi.localIP());

    Serial.println("Done Initialize");

    pixels.setPixelColor(0, pixels.Color(0, 0, 32));
    pixels.show();
}

void loop()
{
}