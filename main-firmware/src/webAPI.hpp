#include <ArduinoJson.h>

void writeJSONResponse(const char *api, int statusCode, JsonDocument &doc);
String getRequestBody(void);
void setupWebAPI(void);
void handleWebAPI(void);