#include <WebServer.h>
#include <Arduino.h>
#include <ArduinoJson.h>

#include <config.hpp>
#include <led.hpp>
#include <webAPI.hpp>

void handlePostCH9329API(void);

void addCH9329API(WebServer *server)
{
    server->on("/ch9329", HTTP_POST, handlePostCH9329API);
}

void setupCH9329(void)
{
    CH9329_SERIAL.setPins(CH9329_RX_PIN, CH9329_TX_PIN);
    CH9329_SERIAL.begin(9600);
}

void sendCH9329Key(uint8_t modifier, uint8_t keycodes[6])
{
    uint8_t buf[14];
    memset(buf, 0, sizeof(buf));

    buf[0] = 0x57;
    buf[1] = 0xAB;
    buf[2] = 0x02;
    buf[3] = 0x00;
    buf[4] = 0x08;
    // モディファイヤ
    buf[5] = modifier;
    buf[6] = 0x00;
    // キーコード1
    buf[7] = keycodes[0];
    // キーコード2
    buf[8] = keycodes[1];
    // キーコード3
    buf[9] = keycodes[2];
    // キーコード4
    buf[10] = keycodes[3];
    // キーコード5
    buf[11] = keycodes[4];
    // キーコード6
    buf[12] = keycodes[5];
    // チェックサム
    uint8_t checksum = 0;
    for (int i = 0; i < 13; i++)
    {
        checksum += buf[i];
    }
    buf[13] = checksum;

    for (int i = 0; i < 14; i++)
    {
        CH9329_SERIAL.write(buf[i]);
    }
}

void handlePostCH9329API(void)
{
    // https://spiceman.jp/arduino-sht31-program/
    Serial.println("POST /ch9329");

    uint8_t modifier = 0x00;
    uint8_t keycodes[6] = {0x7b, 0x00, 0x00, 0x00, 0x00, 0x00};
    sendCH9329Key(modifier, keycodes);

    ledShowColor(LED_COLOR_OPERATION);
    JsonDocument resDoc;

    writeJSONResponse("POST /ch9329", 200, resDoc);
}