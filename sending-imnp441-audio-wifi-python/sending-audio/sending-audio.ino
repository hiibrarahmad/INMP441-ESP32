#include <WiFi.h>
#include <driver/i2s.h>

#define I2S_WS 22
#define I2S_SD 12
#define I2S_SCK 21
#define I2S_PORT I2S_NUM_0
#define BUFFER_LEN 512

const char* ssid = "ibrar";  
const char* password = "123456789";  
const int serverPort = 12345;

WiFiServer server(serverPort);
int16_t sBuffer[BUFFER_LEN];

void setup() {
  Serial.begin(921600);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  
  server.begin();

  // I2S Configuration
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, 
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_LEN,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("Client Connected!");
    
    while (client.connected()) {
      if (client.available()) {
        String command = client.readStringUntil('\n');
        if (command.startsWith("record")) {
          size_t bytes_read;
          for (int i = 0; i < 160; i++) {  // 5 seconds at 16kHz
            i2s_read(I2S_PORT, &sBuffer, sizeof(sBuffer), &bytes_read, portMAX_DELAY);
            client.write((const char *)sBuffer, bytes_read);
          }
        }
      }
    }
    Serial.println("Client Disconnected.");
    client.stop();
  }
}
