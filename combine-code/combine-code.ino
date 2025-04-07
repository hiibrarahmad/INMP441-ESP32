#include <driver/i2s.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>

#define I2S_WS 22
#define I2S_SD 12
#define I2S_SCK 21
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (16 * 1024)
#define RECORD_TIME       (20) //Seconds
#define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)

const char* ssid = "ibrar";
const char* password = "123456789";
WebServer server(80);

File file;
const char filename[] = "/recording.wav";
const int headerSize = 44;

void setup() {
    Serial.begin(115200);
    SPIFFSInit();
    WiFiInit();
    i2sInit();
    xTaskCreate(i2s_adc, "i2s_adc", 1024 * 8, NULL, 1, NULL);
}

void loop() {
    server.handleClient();
}

void SPIFFSInit() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed!");
        while (1) yield();
    }
    SPIFFS.remove(filename);
}

void WiFiInit() {
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

void i2sInit() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = 0,
        .dma_buf_count = 64,
        .dma_buf_len = 1024,
        .use_apll = 1
    };
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };
    i2s_set_pin(I2S_PORT, &pin_config);
}

void i2s_adc(void *arg) {
    size_t bytes_read;
    char* i2s_read_buff = (char*) calloc(I2S_READ_LEN, sizeof(char));
    
    SPIFFS.remove(filename);
    file = SPIFFS.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("File open failed!");
        return;
    }
    byte header[headerSize];
    wavHeader(header, FLASH_RECORD_SIZE);
    file.write(header, headerSize);
    
    Serial.println(" *** Recording Start *** ");
    int flash_wr_size = 0;
    while (flash_wr_size < FLASH_RECORD_SIZE) {
        i2s_read(I2S_PORT, (void*) i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);
        file.write((const byte*) i2s_read_buff, bytes_read);
        flash_wr_size += bytes_read;
        Serial.printf("Recording: %d%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
    }
    file.close();
    Serial.println(" *** Recording Finished *** ");
    
    free(i2s_read_buff);
    delay(5000);
    SPIFFS.remove(filename);
    Serial.println("Old recording deleted, ready for new recording.");
    xTaskCreate(i2s_adc, "i2s_adc", 1024 * 2, NULL, 1, NULL);
    vTaskDelete(NULL);
}

void handleRoot() {
    server.send(200, "text/html", "<h1>ESP32 File Server</h1><a href='/download'>Download recording.wav</a>");
}

void handleDownload() {
    File file = SPIFFS.open(filename, "r");
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

void wavHeader(byte* header, int wavSize) {
    memcpy(header, "RIFF", 4);
    unsigned int fileSize = wavSize + headerSize - 8;
    header[4] = fileSize & 0xFF;
    header[5] = (fileSize >> 8) & 0xFF;
    header[6] = (fileSize >> 16) & 0xFF;
    header[7] = (fileSize >> 24) & 0xFF;
    memcpy(header + 8, "WAVEfmt ", 8);
    memset(header + 16, 0, 20);
    header[16] = 16;
    header[20] = 1;
    header[22] = 1;
    header[24] = I2S_SAMPLE_RATE & 0xFF;
    header[25] = (I2S_SAMPLE_RATE >> 8) & 0xFF;
    header[28] = (I2S_SAMPLE_RATE * I2S_CHANNEL_NUM * I2S_SAMPLE_BITS / 8) & 0xFF;
    header[32] = I2S_CHANNEL_NUM * I2S_SAMPLE_BITS / 8;
    header[34] = I2S_SAMPLE_BITS;
    memcpy(header + 36, "data", 4);
    header[40] = wavSize & 0xFF;
    header[41] = (wavSize >> 8) & 0xFF;
    header[42] = (wavSize >> 16) & 0xFF;
    header[43] = (wavSize >> 24) & 0xFF;
}
