#include <WiFi.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>
#include <time.h>

// WiFi credentials
const char* ssid = "ibrar";
const char* password = "ibrarahmad";

// I2S Configuration
#define I2S_WS 25
#define I2S_SD 32
#define I2S_SCK 33
#define SAMPLE_RATE 16000
#define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#define CHANNELS 1

// Web Server
AsyncWebServer server(80);

// Recording variables
volatile bool isRecording = false;
File currentFile;
String currentFilename;

// WAV Header Structure
struct WavHeader {
    char riff[4] = {'R','I','F','F'};
    uint32_t chunkSize = 0;
    char wave[4] = {'W','A','V','E'};
    char fmt[4] = {'f','m','t',' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = CHANNELS;
    uint32_t sampleRate = SAMPLE_RATE;
    uint32_t byteRate = SAMPLE_RATE * CHANNELS * (BITS_PER_SAMPLE/8);
    uint16_t blockAlign = CHANNELS * (BITS_PER_SAMPLE/8);
    uint16_t bitsPerSample = BITS_PER_SAMPLE;
    char data[4] = {'d','a','t','a'};
    uint32_t dataSize = 0;
};

void setup() {
    Serial.begin(115200);
    
    // Initialize SPIFFS
    if(!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Configure time
    configTime(0, 0, "pool.ntp.org");
    time_t now = time(nullptr);
    while (now < 24*3600*30) { // Wait for valid time
        delay(500);
        now = time(nullptr);
    }

    // Configure I2S
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };
    i2s_set_pin(I2S_NUM_0, &pin_config);

    // Configure Web Server Routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = "<html><body>";
        html += "<h1>ESP32 Audio Recorder</h1>";
        html += "<form action='/record' method='post'>";
        html += isRecording ? 
            "<button style='background:red'>Stop Recording</button>" : 
            "<button style='background:green'>Start Recording</button>";
        html += "</form>";
        html += "<h2>Recordings:</h2><ul>";
        
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while(file){
            if (String(file.name()).endsWith(".wav")) {
                html += "<li>" + String(file.name()) + 
                    " <a href='/download/" + String(file.name()) + "'>Download</a>" +
                    " <a href='/delete/" + String(file.name()) + "' style='color:red'>Delete</a></li>";
            }
            file = root.openNextFile();
        }
        html += "</ul></body></html>";
        request->send(200, "text/html", html);
    });

    server.on("/record", HTTP_POST, [](AsyncWebServerRequest *request){
        if(isRecording) {
            stopRecording();
            request->send(200, "text/plain", "Recording stopped");
        } else {
            startRecording();
            request->send(200, "text/plain", "Recording started: " + currentFilename);
        }
    });

    server.on("/download/*", HTTP_GET, [](AsyncWebServerRequest *request){
        String path = request->url().substring(strlen("/download/"));
        if(SPIFFS.exists(path)) {
            request->send(SPIFFS, path, "audio/wav", true);
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server.on("/delete/*", HTTP_GET, [](AsyncWebServerRequest *request){
        String path = request->url().substring(strlen("/delete/"));
        if(SPIFFS.exists(path)) {
            SPIFFS.remove(path);
            request->send(200, "text/plain", "File deleted: " + path);
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server.begin();
}

void loop() {
    if(isRecording) {
        uint8_t buffer[1024];
        size_t bytesRead = 0;
        esp_err_t result = i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, 0);
        if(result == ESP_OK && bytesRead > 0) {
            currentFile.write(buffer, bytesRead);
        }
    }
}

void startRecording() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
        currentFilename = "/recording_unknown_time.wav";
    } else {
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &timeinfo);
        currentFilename = "/recording_" + String(buffer) + ".wav";
    }

    File file = SPIFFS.open(currentFilename, "w");
    if(!file) {
        Serial.println("Failed to create file");
        return;
    }

    WavHeader header;
    file.write((uint8_t*)&header, sizeof(header));
    file.close();

    currentFile = SPIFFS.open(currentFilename, "a");
    if(!currentFile) {
        Serial.println("Failed to open file for appending");
        return;
    }

    isRecording = true;
    Serial.println("Recording started");
}

void stopRecording() {
    isRecording = false;
    currentFile.close();

    // Update WAV header
    File file = SPIFFS.open(currentFilename, "r+");
    if(file) {
        uint32_t fileSize = file.size();
        
        // Update Chunk Size
        uint32_t chunkSize = fileSize - 8;
        file.seek(4);
        file.write((uint8_t*)&chunkSize, 4);
        
        // Update Data Size
        uint32_t dataSize = fileSize - sizeof(WavHeader);
        file.seek(40);
        file.write((uint8_t*)&dataSize, 4);
        
        file.close();
    }
    
    Serial.println("Recording stopped");
    currentFilename = "";
}