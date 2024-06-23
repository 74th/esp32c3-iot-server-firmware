#include <Arduino.h>

#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>

#include <config.hpp>
#include <led.hpp>
#include <webAPI.hpp>

#define UINT64_STR_LEN 16
#define RECEIVING_MAX_WAIT_MS 5000

const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeout = 50;
const uint8_t kTolerancePercentage = kTolerance;
const uint16_t kMinUnknownSize = 12;

IRsend irsend(IR_SEND_PIN);
IRrecv irrecv(IR_RECV_PIN, kCaptureBufferSize, kTimeout, true);
decode_results results;

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

void setupIR()
{
    irsend.begin();

    irrecv.setUnknownThreshold(kMinUnknownSize);
    irrecv.setTolerance(kTolerancePercentage);
    irrecv.enableIRIn();
}

void handleIRSendAPI(void)
{
    Serial.println("POST /ir/send");
    ledShowColor(LED_COLOR_OPERATION);

    JsonDocument resDoc;
    JsonDocument reqDoc;

    uint32_t now = millis();
    resDoc["ts"] = now;
    resDoc["lib_version"] = _IRREMOTEESP8266_VERSION_STR;
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
    ledShowColor(LED_COLOR_RECEIVE);

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
