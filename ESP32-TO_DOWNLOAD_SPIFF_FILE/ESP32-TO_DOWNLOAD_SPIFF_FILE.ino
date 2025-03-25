#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebServer.h>

const char* ssid = "ibrar";
const char* password = "ibrarahmad";

WebServer server(80);

void handleRoot() {
    server.send(200, "text/html", "<h1>ESP32 File Server</h1><a href='/download'>Download recording.wav</a>");
}

void handleDownload() {
    File file = SPIFFS.open("/recording.wav", "r");
    if (!file) {
        server.send(404, "text/plain", "File Not Found");
        return;
    }
    
    server.sendHeader("Content-Type", "audio/wav");
    server.sendHeader("Content-Disposition", "attachment; filename=recording.wav");
    server.sendHeader("Content-Length", String(file.size()));
    
    server.streamFile(file, "audio/wav");
    file.close();
}

void setup() {
    Serial.begin(115200);
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    server.on("/", handleRoot);
    server.on("/download", handleDownload);
    
    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
}
