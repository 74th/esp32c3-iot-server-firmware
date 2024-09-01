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

#define TYPE_SONY "SONY"
#define TYPE_DAIKIN "DAIKIN"
#define TYPE_DAIKIN2 "DAIKIN2"
#define TYPE_DAIKIN128 "DAIKIN128"
#define TYPE_DAIKIN152 "DAIKIN152"
#define TYPE_DAIKIN160 "DAIKIN160"
#define TYPE_DAIKIN176 "DAIKIN176"
#define TYPE_DAIKIN200 "DAIKIN200"
#define TYPE_DAIKIN216 "DAIKIN216"
#define TYPE_DAIKIN312 "DAIKIN312"
#define TYPE_FUJITSU_AC "FUJITSU_AC"
#define TYPE_MITSUBISHI_AC "MITSUBISHI_AC"
#define TYPE_TOSHIBA_AC "TOSHIBA_AC"
#define TYPE_HITACHI_AC "HITACHI_AC"
#define TYPE_HITACHI_AC1 "HITACHI_AC1"
#define TYPE_HITACHI_AC2 "HITACHI_AC2"
#define TYPE_HITACHI_AC3 "HITACHI_AC3"
#define TYPE_HITACHI_AC264 "HITACHI_AC264"
#define TYPE_HITACHI_AC296 "HITACHI_AC296"
#define TYPE_HITACHI_AC344 "HITACHI_AC344"
#define TYPE_HITACHI_AC424 "HITACHI_AC424"
#define TYPE_SANYO_AC "SANYO_AC"
#define TYPE_SANYO_AC88 "SANYO_AC88"
#define TYPE_SANYO_AC152 "SANYO_AC152"
#define TYPE_SHARP_AC "SHARP_AC"
#define TYPE_SHARP_AC152 "SHARP_AC152"

#define TYPE_NO_SONY 1
#define TYPE_NO_DAIKIN 2
#define TYPE_NO_DAIKIN2 3
#define TYPE_NO_DAIKIN128 4
#define TYPE_NO_DAIKIN152 5
#define TYPE_NO_DAIKIN160 6
#define TYPE_NO_DAIKIN176 7
#define TYPE_NO_DAIKIN200 8
#define TYPE_NO_DAIKIN216 9
#define TYPE_NO_DAIKIN312 10
#define TYPE_NO_FUJITSU_AC 11
#define TYPE_NO_MITSUBISHI_AC 12
#define TYPE_NO_TOSHIBA_AC 13
#define TYPE_NO_HITACHI_AC 14
#define TYPE_NO_HITACHI_AC1 15
#define TYPE_NO_HITACHI_AC2 16
#define TYPE_NO_HITACHI_AC3 17
#define TYPE_NO_HITACHI_AC264 18
#define TYPE_NO_HITACHI_AC296 19
#define TYPE_NO_HITACHI_AC344 20
#define TYPE_NO_HITACHI_AC424 21
#define TYPE_NO_SANYO_AC 22
#define TYPE_NO_SANYO_AC88 23
#define TYPE_NO_SANYO_AC152 24
#define TYPE_NO_SHARP_AC 25
#define TYPE_NO_SHARP_AC152 26

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

    uint16_t typeNo;
    if (strcmp(type, TYPE_SONY) == 0)
    {
        typeNo = TYPE_NO_SONY;
    }
    else if (strcmp(type, TYPE_DAIKIN) == 0)
    {
        typeNo = TYPE_NO_DAIKIN;
    }
    else if (strcmp(type, TYPE_DAIKIN2) == 0)
    {
        typeNo = TYPE_NO_DAIKIN2;
    }
    else if (strcmp(type, TYPE_DAIKIN128) == 0)
    {
        typeNo = TYPE_NO_DAIKIN128;
    }
    else if (strcmp(type, TYPE_DAIKIN152) == 0)
    {
        typeNo = TYPE_NO_DAIKIN152;
    }
    else if (strcmp(type, TYPE_DAIKIN160) == 0)
    {
        typeNo = TYPE_NO_DAIKIN160;
    }
    else if (strcmp(type, TYPE_DAIKIN176) == 0)
    {
        typeNo = TYPE_NO_DAIKIN176;
    }
    else if (strcmp(type, TYPE_DAIKIN200) == 0)
    {
        typeNo = TYPE_NO_DAIKIN200;
    }
    else if (strcmp(type, TYPE_DAIKIN216) == 0)
    {
        typeNo = TYPE_NO_DAIKIN216;
    }
    else if (strcmp(type, TYPE_DAIKIN312) == 0)
    {
        typeNo = TYPE_NO_DAIKIN312;
    }
    else if (strcmp(type, TYPE_FUJITSU_AC) == 0)
    {
        typeNo = TYPE_NO_FUJITSU_AC;
    }
    else if (strcmp(type, TYPE_MITSUBISHI_AC) == 0)
    {
        typeNo = TYPE_NO_MITSUBISHI_AC;
    }
    else if (strcmp(type, TYPE_TOSHIBA_AC) == 0)
    {
        typeNo = TYPE_NO_TOSHIBA_AC;
    }
    else if (strcmp(type, TYPE_HITACHI_AC) == 0)
    {
        typeNo = TYPE_NO_HITACHI_AC;
    }
    else if (strcmp(type, TYPE_HITACHI_AC1) == 0)
    {
        typeNo = TYPE_NO_HITACHI_AC1;
    }
    else if (strcmp(type, TYPE_HITACHI_AC2) == 0)
    {
        typeNo = TYPE_NO_HITACHI_AC2;
    }
    else if (strcmp(type, TYPE_HITACHI_AC3) == 0)
    {
        typeNo = TYPE_NO_HITACHI_AC3;
    }
    else if (strcmp(type, TYPE_HITACHI_AC264) == 0)
    {
        typeNo = TYPE_NO_HITACHI_AC264;
    }
    else if (strcmp(type, TYPE_HITACHI_AC296) == 0)
    {
        typeNo = TYPE_NO_HITACHI_AC296;
    }
    else if (strcmp(type, TYPE_HITACHI_AC344) == 0)
    {
        typeNo = TYPE_NO_HITACHI_AC344;
    }
    else if (strcmp(type, TYPE_HITACHI_AC424) == 0)
    {
        typeNo = TYPE_NO_HITACHI_AC424;
    }
    else if (strcmp(type, TYPE_SANYO_AC) == 0)
    {
        typeNo = TYPE_NO_SANYO_AC;
    }
    else if (strcmp(type, TYPE_SHARP_AC) == 0)
    {
        typeNo = TYPE_NO_SHARP_AC;
    }
    else if (strcmp(type, TYPE_SHARP_AC152) == 0)
    {
        typeNo = TYPE_NO_SHARP_AC152;
    }
    else
    {
        resDoc["error"] = "unknown type.";
        writeJSONResponse("POST /ir/send", 401, resDoc);
        return;
    }

    Serial.printf("type: %s, type_no: %d, hex: %s\r\n", type, typeNo, hexData);

    unsigned char data[64];
    uint64_t data_u64;
    size_t size;
    uint16_t data_len;

    switch (typeNo)
    {
    case TYPE_NO_SONY:
        data_u64 = hexStringToUint64(hexData);
        break;
    case TYPE_NO_DAIKIN:
    case TYPE_NO_DAIKIN2:
    case TYPE_NO_DAIKIN128:
    case TYPE_NO_DAIKIN152:
    case TYPE_NO_DAIKIN160:
    case TYPE_NO_DAIKIN176:
    case TYPE_NO_DAIKIN200:
    case TYPE_NO_DAIKIN216:
    case TYPE_NO_DAIKIN312:
    case TYPE_NO_FUJITSU_AC:
    case TYPE_NO_MITSUBISHI_AC:
    case TYPE_NO_TOSHIBA_AC:
    case TYPE_NO_HITACHI_AC:
    case TYPE_NO_HITACHI_AC1:
    case TYPE_NO_HITACHI_AC2:
    case TYPE_NO_HITACHI_AC3:
    case TYPE_NO_HITACHI_AC264:
    case TYPE_NO_HITACHI_AC296:
    case TYPE_NO_HITACHI_AC344:
    case TYPE_NO_HITACHI_AC424:
    case TYPE_NO_SANYO_AC:
    case TYPE_NO_SHARP_AC:
    case TYPE_NO_SHARP_AC152:
        hexStringToByteArray(hexData, data, &size);
        Serial.printf("data: %llx\r\n", data);
        break;
    }

    switch (typeNo)
    {
    case TYPE_NO_SONY:
        if (data_u64 <= 1 << 12)
        {
            data_len = 12;
        }
        else if (data_u64 <= 1 << 15)
        {
            data_len = 15;
        }
        else
        {
            data_len = 20;
        }
        irsend.sendSony(data_u64, data_len, 2);
        break;
    case TYPE_NO_DAIKIN:
        irsend.sendDaikin(data, size);
        break;
    case TYPE_NO_DAIKIN2:
        irsend.sendDaikin2(data, size);
        break;
    case TYPE_NO_DAIKIN128:
        irsend.sendDaikin128(data, size);
        break;
    case TYPE_NO_DAIKIN152:
        irsend.sendDaikin152(data, size);
        break;
    case TYPE_NO_DAIKIN160:
        irsend.sendDaikin160(data, size);
        break;
    case TYPE_NO_DAIKIN176:
        irsend.sendDaikin176(data, size);
        break;
    case TYPE_NO_DAIKIN200:
        irsend.sendDaikin200(data, size);
        break;
    case TYPE_NO_DAIKIN216:
        irsend.sendDaikin216(data, size);
        break;
    case TYPE_NO_DAIKIN312:
        irsend.sendDaikin312(data, size);
        break;
    case TYPE_NO_FUJITSU_AC:
        irsend.sendFujitsuAC(data, size);
        break;
    case TYPE_NO_MITSUBISHI_AC:
        irsend.sendMitsubishiAC(data, size);
        break;
    case TYPE_NO_TOSHIBA_AC:
        irsend.sendToshibaAC(data, size);
        break;
    case TYPE_NO_HITACHI_AC:
        irsend.sendHitachiAC(data, size);
        break;
    case TYPE_NO_HITACHI_AC1:
        irsend.sendHitachiAC1(data, size);
        break;
    case TYPE_NO_HITACHI_AC2:
        irsend.sendHitachiAC2(data, size);
        break;
    case TYPE_NO_HITACHI_AC3:
        irsend.sendHitachiAc3(data, size);
        break;
    case TYPE_NO_HITACHI_AC264:
        irsend.sendHitachiAc264(data, size);
        break;
    case TYPE_NO_HITACHI_AC296:
        irsend.sendHitachiAc296(data, size);
        break;
    case TYPE_NO_HITACHI_AC344:
        irsend.sendHitachiAc344(data, size);
        break;
    case TYPE_NO_HITACHI_AC424:
        irsend.sendHitachiAc424(data, size);
        break;
    case TYPE_NO_SANYO_AC:
        irsend.sendSanyoAc(data, size);
        break;
    case TYPE_NO_SANYO_AC88:
        irsend.sendSanyoAc88(data, size);
        break;
    case TYPE_NO_SANYO_AC152:
        irsend.sendSanyoAc152(data, size);
        break;
    case TYPE_NO_SHARP_AC:
        irsend.sendSharpAc(data, size);
        break;
    case TYPE_NO_SHARP_AC152:
        irsend.sendSharpAc(data, size);
        break;
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
