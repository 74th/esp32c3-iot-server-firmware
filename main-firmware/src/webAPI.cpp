#include <Arduino.h>
#include <WebServer.h>

#include <ArduinoJson.h>

#include <led.hpp>
#include <ir.hpp>

#ifdef ENABLE_SHT31
#include <sht31.hpp>
#endif

#ifdef ENABLE_CH9329
#include <ch9329.hpp>
#endif

WebServer server(80);

void handleRootAPI(void)
{
    server.send(200, "application/json", "{}");
}

void handleNotFoundAPI(void)
{
    server.send(404, "application/json", "{\"message\": \"Not Found.\"}");
}

void writeJSONResponse(const char *api, int statusCode, JsonDocument &doc)
{
    char bodyBuf[1024 * 2];
    size_t bodySize = serializeJson(doc, bodyBuf, sizeof(bodyBuf));
    server.send(statusCode, "application/json", bodyBuf);

    Serial.printf("response_body: %s\r\n", bodyBuf);
    Serial.printf("POST /ir/send %d\r\n", statusCode);
    ledShowColor(LED_COLOR_NORMAL);
}

String getRequestBody(void)
{
    return server.arg("plain");
}

void setupWebAPI()
{
    // Webサーバの開始
    server.on("/", HTTP_GET, handleRootAPI);
    server.on("/ir/receive", HTTP_GET, handleIRDecodeAPI);
    server.on("/ir/send", HTTP_POST, handleIRSendAPI);

#ifdef ENABLE_SHT31
    addSHT31API(&server);
#endif

#ifdef ENABLE_CH9329
    addCH9329API(&server);
#endif

    server.onNotFound(handleNotFoundAPI);
    server.begin();
}

void handleWebAPI(void)
{
    server.handleClient();
}