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

void handleRootAPI(void);
void handleNotFoundAPI(void);
void handleIRDecodeAPI(void);

#define LED_PIN GPIO_NUM_2
#define IR_SEND_PIN GPIO_NUM_5
#define IR_RECV_PIN GPIO_NUM_4

#define COLOR_SETUP pixels.Color(0, 32, 0)
#define COLOR_NORMAL pixels.Color(0, 0, 32)
#define COLOR_RECEIVE pixels.Color(32, 32, 0)

#define RECEIVING_MAX_WAIT_MS 5000

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

void setupIR()
{
    irsend.begin();
    irrecv.setUnknownThreshold(kMinUnknownSize);
    irrecv.setTolerance(kTolerancePercentage);
    irrecv.enableIRIn();
}

void setupServer()
{
    // Webサーバの開始
    server.on("/", HTTP_GET, handleRootAPI);
    server.on("/ir/receive", HTTP_GET, handleIRDecodeAPI);
    server.onNotFound(handleNotFoundAPI);
    server.begin();
}

void setup()
{
    Serial.begin(115200);

    Serial.println("Start Initialize");

    pixels.begin();
    pixels.setPixelColor(0, COLOR_SETUP);
    pixels.show();

    setupWiFi();
    setupIR();
    setupServer();

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

    char bodyBuf[1024 * 2];
    size_t bodySize = serializeJson(doc, bodyBuf, sizeof(bodyBuf));
    Serial.println(bodyBuf);
    server.send(200, "application/json", bodyBuf);

    Serial.println("GET /ir/receive 200");
    pixels.setPixelColor(0, COLOR_NORMAL);
    pixels.show();
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

void loop()
{
    // handleIRDecode();
    server.handleClient();
    handleMonitorOutput();
}