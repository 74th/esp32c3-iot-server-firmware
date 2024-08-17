#include <WebServer.h>
#include <Wire.h>
#include <ArduinoJson.h>

#include <sht31.hpp>
#include <led.hpp>
#include <webAPI.hpp>

#define SHT31_I2C_ADDR 0x44

void handleGetSHT31API(void);

void addSHT31API(WebServer *server)
{
    server->on("/sht31", HTTP_GET, handleGetSHT31API);
}

void setupSHT31(void)
{
    // ソフトリセット
    Wire.beginTransmission(SHT31_I2C_ADDR);
    Wire.write(0x30);
    Wire.write(0xA2);
    Wire.endTransmission();

    delay(300);

    // ステータスレジスタ消去
    Wire.beginTransmission(SHT31_I2C_ADDR);
    Wire.write(0x30);
    Wire.write(0x41);
    Wire.endTransmission();

    delay(300);
}

void handleGetSHT31API(void)
{
    // https://spiceman.jp/arduino-sht31-program/
    Serial.println("GET /sht31");
    ledShowColor(LED_COLOR_OPERATION);

    JsonDocument resDoc;

    uint32_t now = millis();
    resDoc["ts"] = now;
    resDoc["lib_version"] = "1.0.0";
    resDoc["success"] = false;

    unsigned int dac[6];
    unsigned int i, t, h;
    float temperature, humidity;

    Wire.beginTransmission(SHT31_I2C_ADDR);
    Wire.write(0x24);
    Wire.write(0x00);
    Wire.endTransmission();

    delay(300);

    Wire.requestFrom(SHT31_I2C_ADDR, 6);
    for (i = 0; i < 6; i++)
    {
        dac[i] = Wire.read();
    }
    Wire.endTransmission();

    t = (dac[0] << 8) | dac[1];                      // 1Byte目のデータを8bit左にシフト、OR演算子で2Byte目のデータを結合して、tに代入
    temperature = (float)(t) * 175 / 65535.0 - 45.0; // 温度の計算、temperatureに代入
    h = (dac[3] << 8) | dac[4];                      ////4Byte目のデータを8bit左にシフト、OR演算子で5Byte目のデータを結合して、hに代入
    humidity = (float)(h) / 65535.0 * 100.0;         // 湿度の計算、humidityに代入

    resDoc["data"]["temperature"] = temperature;
    resDoc["data"]["humidity"] = humidity;

    Serial.print("temperature：");
    Serial.print(temperature);
    Serial.print(" humidity：");
    Serial.println(humidity);

    resDoc["success"] = true;

    writeJSONResponse("GET /sht31", 200, resDoc);
}