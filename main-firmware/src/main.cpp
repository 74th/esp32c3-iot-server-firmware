#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <WebServer.h>

#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

#include <ssid.hpp>
#include <config.hpp>
#include <led.hpp>
#include <ir.hpp>
#include <webAPI.hpp>

#ifdef ENABLE_SHT31
#include <sht31.hpp>
#endif

#ifdef ENABLE_CH9329
#include <ch9329.hpp>
#endif

const char *ssid = WIFI_SSID;
const char *pass = WIFI_PASSWORD;

// WiFiの接続
void setupWiFi()
{
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }
    Serial.print("IP:");
    Serial.println(WiFi.localIP());
}

void setupI2C()
{
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
}

void setupButton()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void setup()
{
    delay(3000);
    Serial.begin(115200);

    Serial.println("Start Initialize");

    ledSetup();

    setupWiFi();
    setupI2C();
    setupIR();
    setupWebAPI();
    setupButton();
#ifdef ENABLE_SHT31
    setupSHT31();
#endif
#ifdef ENABLE_CH9329
    setupCH9329();
#endif

    ledShowColor(LED_COLOR_NORMAL);

    Serial.println("Done Initialize");
}

uint32_t latestStatMS = 0;

void handleMonitorOutput()
{
    uint32_t now = millis();

    if (now < latestStatMS + 1000)
    {
        return;
    }

    latestStatMS = now;

    Serial.print("IP:");
    Serial.println(WiFi.localIP());
}

// true: released, false: pushed
bool buttonState = false;

void handleButton()
{
    bool current = digitalRead(BUTTON_PIN);
    bool nowReleased = current && !buttonState;
    buttonState = current;
    if (!nowReleased)
        return;

    Serial.println("Button Pushed");
    ledShowColor(LED_COLOR_OPERATION);

    delay(500);

    ledShowColor(LED_COLOR_NORMAL);
}

void loop()
{
    handleWebAPI();
    handleMonitorOutput();
    handleButton();
}