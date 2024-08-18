#include <WebServer.h>
#include <Arduino.h>
#include <ArduinoJson.h>

#include <config.hpp>
#include <led.hpp>
#include <webAPI.hpp>

#define _CH9329_LIB_VERSION "1.0.0"

void handlePostCH9329KeyboardAPI(void);

void addCH9329API(WebServer *server)
{
    server->on("/ch9329/keyboard", HTTP_POST, handlePostCH9329KeyboardAPI);
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
    buf[2] = 0x00;
    buf[3] = 0x02;
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

void sendCH9329FreeKey()
{
    uint8_t buf[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    sendCH9329Key(0x00, buf);
}

void handlePostCH9329KeyboardAPI(void)
{
    // https://spiceman.jp/arduino-sht31-program/
    Serial.println("POST /ch9329/keyboard");
    ledShowColor(LED_COLOR_OPERATION);

    JsonDocument resDoc;
    JsonDocument reqDoc;

    uint32_t now = millis();
    resDoc["ts"] = now;
    resDoc["lib_version"] = _CH9329_LIB_VERSION;
    resDoc["success"] = false;

    String requestBody = getRequestBody();
    Serial.printf("request body: %s\r\n", requestBody.c_str());
    DeserializationError error = deserializeJson(reqDoc, requestBody);
    if (error)
    {
        resDoc["error"] = "failed to deserialize Json.";
        writeJSONResponse("POST /ir/send", 401, resDoc);
        return;
    }

    const bool reqCtrl = reqDoc["ctrl"];
    const bool reqShift = reqDoc["shift"];
    const bool reqAlt = reqDoc["alt"];
    const bool reqWin = reqDoc["win"];
    const JsonArray reqKeycodeArray = reqDoc["keycode"];

    uint8_t modifier = 0x00;
    if (reqCtrl)
        modifier |= 0x01;
    if (reqShift)
        modifier |= 0x02;
    if (reqAlt)
        modifier |= 0x04;
    if (reqWin)
        modifier |= 0x08;

    Serial.printf("keycode: ");
    uint8_t keycodes[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    for (int i = 0; i < reqKeycodeArray.size(); i++)
    {
        keycodes[i] = reqKeycodeArray[i].as<uint8_t>();
        Serial.printf("0x%02X ", keycodes[i]);
    }
    Serial.printf("\r\n");

    sendCH9329Key(modifier, keycodes);

    delay(100);

    sendCH9329FreeKey();

    resDoc["success"] = true;

    writeJSONResponse("POST /ch9329", 200, resDoc);
}