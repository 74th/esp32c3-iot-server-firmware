#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>

#include <ssid.h>
#include <Wire.h>

#define UINT64_STR_LEN 16

void handleRootAPI(void);
void handleNotFoundAPI(void);
void handleIRSendAPI(void);
void handleIRDecodeAPI(void);
#ifdef ENABLE_SHT31
void handleGetSHT31API(void);
#endif

#define BUTTON_PIN GPIO_NUM_3
#define LED_PIN GPIO_NUM_2
#define IR_SEND_PIN GPIO_NUM_5
#define IR_RECV_PIN GPIO_NUM_4

#define I2C_SCL_PIN GPIO_NUM_0
#define I2C_SDA_PIN GPIO_NUM_1

#define COLOR_SETUP pixels.Color(0, 32, 0)
#define COLOR_NORMAL pixels.Color(0, 0, 32)
#define COLOR_RECEIVE pixels.Color(32, 32, 0)
#define COLOR_OPERATION pixels.Color(0, 32, 32)

#define RECEIVING_MAX_WAIT_MS 5000

#define SHT31_I2C_ADDR 0x44

const char *ssid = WIFI_SSID;
const char *pass = WIFI_PASSWORD;

WebServer server(80);
Adafruit_NeoPixel pixels(1, LED_PIN, NEO_GRB + NEO_KHZ800);
IRsend irsend(IR_SEND_PIN);

const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeout = 50;
const uint8_t kTolerancePercentage = kTolerance;
const uint16_t kMinUnknownSize = 12;
IRrecv irrecv(IR_RECV_PIN, kCaptureBufferSize, kTimeout, true);

decode_results results;

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

void setupIR()
{
    irsend.begin();

    irrecv.setUnknownThreshold(kMinUnknownSize);
    irrecv.setTolerance(kTolerancePercentage);
    irrecv.enableIRIn();
}

#ifdef ENABLE_SHT31
void setupSHT31()
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

    // 連続測定・繰り返し精度レベル高・測定頻度1mps
    // Wire.beginTransmission(SHT31_I2C_ADDR);
    // Wire.write(0x21);
    // Wire.write(0x30);
    // Wire.endTransmission();
}
#endif ENABLE_SHT31

void setupServer()
{
    // Webサーバの開始
    server.on("/", HTTP_GET, handleRootAPI);
    server.on("/ir/receive", HTTP_GET, handleIRDecodeAPI);
    server.on("/ir/send", HTTP_POST, handleIRSendAPI);
#ifdef ENABLE_SHT31
    server.on("/sht31", HTTP_GET, handleGetSHT31API);
#endif
    server.onNotFound(handleNotFoundAPI);
    server.begin();
}

void setupButton()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void setup()
{
    Serial.begin(115200);

    Serial.println("Start Initialize");

    pixels.begin();
    pixels.setPixelColor(0, COLOR_SETUP);
    pixels.show();

    setupWiFi();
    setupI2C();
    setupIR();
    setupServer();
    setupButton();
#ifdef ENABLE_SHT31
    setupSHT31();
#endif

    pixels.setPixelColor(0, COLOR_NORMAL);
    pixels.show();

    Serial.println("Done Initialize");
}

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
    pixels.setPixelColor(0, COLOR_NORMAL);
    pixels.show();
}

uint64_t hexStringToUint64(const char *hexString)
{
    if (hexString[0] == '0' && (hexString[1] == 'x' || hexString[1] == 'X'))
    {
        hexString += 2;
    }

    uint64_t result = strtoull(hexString, NULL, 16);

    return result;
}

// 16進数文字を対応する数値に変換
unsigned char hexCharToValue(char hexChar)
{
    if ('0' <= hexChar && hexChar <= '9')
    {
        return hexChar - '0';
    }
    else if ('a' <= hexChar && hexChar <= 'f')
    {
        return hexChar - 'a' + 10;
    }
    else if ('A' <= hexChar && hexChar <= 'F')
    {
        return hexChar - 'A' + 10;
    }
    else
    {
        return 0;
    }
}

// HEX文字列をバイト列に変換する関数
void hexStringToByteArray(const char *hexString, unsigned char *byteArray, size_t *byteArraySize)
{
    if (hexString[0] == '0' && (hexString[1] == 'x' || hexString[1] == 'X'))
    {
        hexString += 2;
    }

    size_t hexLen = strlen(hexString);
    *byteArraySize = hexLen / 2;

    for (size_t i = 0; i < hexLen / 2; ++i)
    {
        byteArray[i] = (hexCharToValue(hexString[2 * i]) << 4) | hexCharToValue(hexString[2 * i + 1]);
    }
}

void handleIRSendAPI(void)
{
    Serial.println("POST /ir/send");
    pixels.setPixelColor(0, COLOR_OPERATION);
    pixels.show();

    JsonDocument resDoc;
    JsonDocument reqDoc;

    uint32_t now = millis();
    resDoc["ts"] = now;
    resDoc["lib_version"] = _IRREMOTEESP8266_VERSION_STR;
    resDoc["success"] = false;

    String requestBody = server.arg("plain");
    Serial.printf("request body: %s\r\n", requestBody.c_str());
    DeserializationError error = deserializeJson(reqDoc, requestBody);
    if (error)
    {
        resDoc["error"] = "failed to deserialize Json.";
        writeJSONResponse("POST /ir/send", 401, resDoc);
        return;
    }

    uint64_t data[32];
    const char *hexData = reqDoc["hex"];
    if (hexData == NULL || strlen(hexData) == 0)
    {
        resDoc["error"] = "hex is required.";
        writeJSONResponse("POST /ir/send", 401, resDoc);
        return;
    }

    size_t dataSize;

    const char *type = reqDoc["type"];
    if (type == NULL || strlen(type) == 0)
    {
        resDoc["error"] = "type is required.";
        writeJSONResponse("POST /ir/send", 401, resDoc);
        return;
    }

    Serial.printf("type: %s, hex: %s\r\n", type, hexData);

    if (strcmp(type, "SONY") == 0)
    {
        uint64_t data = hexStringToUint64(hexData);
        Serial.printf("data: %llx\r\n", data);
        irsend.sendSony(data, 12, 2);
    }
    else if (strcmp(type, "MITSUBISHI_AC") == 0)
    {
        if (strlen(hexData) > 32 * UINT64_STR_LEN)
        {
            resDoc["error"] = "hex is too long.";
            writeJSONResponse("POST /ir/send", 401, resDoc);
            return;
        }

        unsigned char data[64];
        size_t size;
        hexStringToByteArray(hexData, data, &size);
        Serial.printf("data: %llx\r\n", data);

        irsend.sendMitsubishiAC(data, size);
    }
    else
    {
        resDoc["error"] = "unknown type.";
        writeJSONResponse("POST /ir/send", 401, resDoc);
        return;
    }

    resDoc["success"] = true;

    writeJSONResponse("POST /ir/send", 200, resDoc);
}

void handleIRDecodeAPI(void)
{
    Serial.println("GET /ir/receive");
    pixels.setPixelColor(0, COLOR_RECEIVE);
    pixels.show();

    JsonDocument doc;

    doc["lib_version"] = _IRREMOTEESP8266_VERSION_STR;

    for (uint32_t start = millis(); true;)
    {
        if (millis() > start + RECEIVING_MAX_WAIT_MS)
        {
            doc["available"] = false;
            break;
        }

        if (!irrecv.decode(&results))
        {
            continue;
        }

        doc["available"] = true;

        // https://github.com/crankyoldgit/IRremoteESP8266/blob/2bfdf9719250471f004b720e81c14ab00a244836/examples/IRrecvDumpV3/IRrecvDumpV3.ino#L164-L190
        // Check if we got an IR message that was to big for our capture buffer.
        if (results.overflow)
            Serial.printf(D_WARN_BUFFERFULL "\n", kCaptureBufferSize);

        // Display the tolerance percentage if it has been change from the default.
        // if (kTolerancePercentage != kTolerance)
        //     Serial.printf(D_STR_TOLERANCE " : %d%%\n", kTolerancePercentage);

        // Display the basic output of what we found.
        Serial.print(resultToHumanReadableBasic(&results));
        doc["data"]["type"] = typeToString(results.decode_type, results.repeat);
        doc["data"]["hex"] = resultToHexidecimal(&results);
        // Display any extra A/C info if we have it.
        String description = IRAcUtils::resultAcToString(&results);
        if (description.length())
            Serial.println(D_STR_MESGDESC ": " + description);
        doc["data"]["mes"] = description;
        yield(); // Feed the WDT as the text output can take a while to print.

        // Output the results as source code
        // Serial.println(resultToSourceCode(&results));
        // Serial.println(); // Blank line between entries
        // yield();          // Feed the WDT (again)

        break;
    }
    uint32_t now = millis();

    doc["ts"] = now;

    writeJSONResponse("GET /ir/receive", 200, doc);
}

#ifdef ENABLE_SHT31
void handleGetSHT31API(void)
{
    // https://spiceman.jp/arduino-sht31-program/
    Serial.println("GET /sht31");
    pixels.setPixelColor(0, COLOR_OPERATION);
    pixels.show();

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
#endif

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
    pixels.setPixelColor(0, COLOR_OPERATION);

    irsend.sendSony(0xa90, 12, 2); // 12 bits & 2 repeats

    pixels.setPixelColor(0, COLOR_NORMAL);
}

void loop()
{
    server.handleClient();
    handleMonitorOutput();
    handleButton();
}