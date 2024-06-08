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

#define LED_PIN GPIO_NUM_2
#define IR_SEND_PIN GPIO_NUM_5
#define IR_RECV_PIN GPIO_NUM_4

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

// IR受信開始
void setupIR()
{
    irrecv.setUnknownThreshold(kMinUnknownSize);
    irrecv.setTolerance(kTolerancePercentage); // Override the default tolerance.
    irrecv.enableIRIn();                       // Start the receiver
}

void setup()
{
    Serial.begin(115200);

    Serial.println("Start Initialize");

    pixels.begin();
    pixels.setPixelColor(0, pixels.Color(0, 32, 0));
    pixels.show();

    setupWiFi();
    setupIR();

    pixels.setPixelColor(0, pixels.Color(0, 0, 32));
    pixels.show();

    Serial.println("Done Initialize");
}

uint32_t latestStatMS = 0;

void handleIRDecode()
{
    if (!irrecv.decode(&results))
    {
        return;
    }
    // Display a crude timestamp.
    uint32_t now = millis();
    Serial.printf(D_STR_TIMESTAMP " : %06u.%03u\n", now / 1000, now % 1000);
    // Check if we got an IR message that was to big for our capture buffer.
    if (results.overflow)
        Serial.printf(D_WARN_BUFFERFULL "\n", kCaptureBufferSize);
    // Display the library version the message was captured with.
    Serial.println(D_STR_LIBRARY "   : v" _IRREMOTEESP8266_VERSION_STR "\n");
    // Display the tolerance percentage if it has been change from the default.
    if (kTolerancePercentage != kTolerance)
        Serial.printf(D_STR_TOLERANCE " : %d%%\n", kTolerancePercentage);
    // Display the basic output of what we found.
    Serial.print(resultToHumanReadableBasic(&results));
    // Display any extra A/C info if we have it.
    String description = IRAcUtils::resultAcToString(&results);
    if (description.length())
        Serial.println(D_STR_MESGDESC ": " + description);
    yield(); // Feed the WDT as the text output can take a while to print.
    // Output the results as source code
    Serial.println(resultToSourceCode(&results));
    Serial.println(); // Blank line between entries
    yield();          // Feed the WDT (again)
}

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
    handleIRDecode();
    handleMonitorOutput();
}