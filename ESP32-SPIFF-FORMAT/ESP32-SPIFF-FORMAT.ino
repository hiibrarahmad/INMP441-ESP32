#include <SPIFFS.h>

void setup() {
  Serial.begin(115200);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialisation failed!");
    return;
  }

  Serial.println("SPIFFS Mounted Successfully!");

  // Format the SPIFFS
  Serial.println("Formatting SPIFFS...");
  bool formatted = SPIFFS.format();
  
  if (formatted) {
    Serial.println("SPIFFS formatted successfully.");
  } else {
    Serial.println("SPIFFS formatting failed.");
  }
}

void loop() {
  // Nothing to do here
}
