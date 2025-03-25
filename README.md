# 🎤 ESP32 INMP441 Audio Recording & Playback 🚀

---

## 📌 Overview

This project demonstrates how to use the **ESP32** to record high-quality audio from an **INMP441** microphone and save it as a **WAV** file in SPIFFS. The recorded file can be accessed via an **HTTP web server** hosted on the ESP32, allowing users to download and listen to it remotely.

🔹 **Ideal for:** IoT audio projects, smart voice recorders, and real-time audio processing.  
🔹 **Built-in Web UI** for seamless file management and playback.

---

## ✨ Features

✅ **High-Quality Audio Capture**: Records crystal-clear audio using **I2S** protocol.  
✅ **SPIFFS Storage**: Saves audio files in **ESP32's internal storage**.  
✅ **Web-Based Interface**: Download and manage audio files directly from your browser.  
✅ **WAV Format Support**: Ensures compatibility with most media players.  
✅ **Real-Time Status Updates**: Displays recording progress via the serial monitor.  
✅ **User-Friendly Web Server**: Hosted on the ESP32 for easy file access.

---

## 🛠 Hardware Requirements

- 🟢 **ESP32 Dev Board**
- 🎤 **INMP441 I2S Microphone**
- 🔌 **USB Cable** (for programming & power)

---

## 🔌 Wiring Diagram

| INMP441 Pin | ESP32 Pin |
| ----------- | --------- |
| **VCC**     | 3.3V      |
| **GND**     | GND       |
| **WS**      | GPIO 25   |
| **SCK**     | GPIO 33   |
| **SD**      | GPIO 32   |

---

## 📥 Installation & Setup

### 🔹 1️⃣ Clone the Repository

```sh
 git clone https://github.com/yourusername/ESP32-INMP441-Audio.git
 cd ESP32-INMP441-Audio
```

### 🔹 2️⃣ Install Required Libraries

Make sure you have the following libraries installed in **Arduino IDE** or **PlatformIO**:

- `WiFi.h` (for network communication)
- `FS.h` (for file handling)
- `SPIFFS.h` (for storage)
- `driver/i2s.h` (for I2S communication)
- `WebServer.h` (for hosting the web interface)

### 🔹 3️⃣ Upload the Code

Open `ESP32_Audio_Recorder.ino` in the **Arduino IDE** and upload it to your ESP32.

---

## 📝 Code Breakdown

# Detailed Code Breakdown

## 🟢 1. **Audio Recording Code**

This code captures audio using **I2S** and saves it as a **WAV file** in SPIFFS (ESP32's internal file system).

### 📌 **Includes & Definitions**

```cpp
#include <driver/i2s.h>
#include <SPIFFS.h>
```

- **driver/i2s.h**: Provides I2S communication.
- **SPIFFS.h**: Manages the internal file system.

```cpp
#define I2S_WS 25
#define I2S_SD 32
#define I2S_SCK 33
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (16 * 1024)
#define RECORD_TIME       (20) // Seconds
#define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)
```

- Defines I2S pin connections.
- Sets up sampling parameters: **16KHz, 16-bit, Mono.**
- **FLASH_RECORD_SIZE** determines the total recording size.

### 📌 **Setup Function**

```cpp
void setup() {
  Serial.begin(115200);
  SPIFFSInit();
  i2sInit();
  xTaskCreate(i2s_adc, "i2s_adc", 2048, NULL, 1, NULL);
}
```

- Initializes Serial for debugging.
- Calls **SPIFFSInit()** to initialize storage.
- Calls **i2sInit()** to configure I2S.
- Creates a **task (i2s_adc)** for audio recording.

### 📌 **SPIFFS Initialization**

```cpp
void SPIFFSInit(){
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS initialization failed!");
    while(1) yield();
  }
  SPIFFS.remove("/recording.wav");
  file = SPIFFS.open("/recording.wav", FILE_WRITE);
  byte header[44];
  wavHeader(header, FLASH_RECORD_SIZE);
  file.write(header, 44);
}
```

- Initializes SPIFFS.
- Removes any previous **recording.wav** file.
- Creates a new **WAV** file and writes the **header.**

### 📌 **I2S Configuration**

```cpp
void i2sInit(){
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
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
```

- Configures **I2S** as Master, 16-bit PCM, and DMA buffering.
- Installs **I2S driver**.
- Sets **I2S pin mappings.**

### 📌 **Audio Recording Task**

```cpp
void i2s_adc(void *arg){
    int i2s_read_len = I2S_READ_LEN;
    int flash_wr_size = 0;
    size_t bytes_read;
    char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
    uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

    while (flash_wr_size < FLASH_RECORD_SIZE) {
        i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
        file.write((const byte*) i2s_read_buff, i2s_read_len);
        flash_wr_size += i2s_read_len;
    }
    file.close();
    free(i2s_read_buff);
    free(flash_write_buff);
    vTaskDelete(NULL);
}
```

- Reads **audio data** from I2S.
- Saves data to **SPIFFS** until the **recording size** is met.
- Cleans up memory after completion.

## 🔵 2. **Web Server for File Download**

### 📌 **WiFi & Server Setup**

```cpp
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebServer.h>

const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";
WebServer server(80);
```

- **Defines WiFi credentials**.
- **Creates an HTTP server** on port **80**.

### 📌 **Handling Requests**

```cpp
void handleRoot() {
    server.send(200, "text/html", "<h1>ESP32 File Server</h1><a href='/download'>Download recording.wav</a>");
}
```

- Serves a simple HTML page with a download link.

```cpp
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
```

- Opens **recording.wav** from **SPIFFS**.
- Sends the file as a **WAV attachment**.

### 📌 **Setup Function**

```cpp
void setup() {
    Serial.begin(115200);
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }
    server.on("/", handleRoot);
    server.on("/download", handleDownload);
    server.begin();
}
```

- **Connects to WiFi**.
- **Initializes the Web Server**.

### 📌 **Main Loop**

```cpp
void loop() {
    server.handleClient();
}
```

- **Handles incoming client requests**.

## 🎯 **Summary**
- **Records audio** using I2S.
- **Saves data** to SPIFFS as a WAV file.
- **Hosts a web server** to download the recorded file.


## 🚀 How to Use

1️⃣ **Power up the ESP32** with the INMP441 microphone connected.  
2️⃣ **Recording starts automatically**, and saves the file as `recording.wav`.  
3️⃣ **Connect to ESP32's WiFi** network (`SSID: ibrar, Password: ibrarahmad`).  
4️⃣ Open a web browser and visit `http://<ESP32_IP>`.  
5️⃣ Click **Download recording.wav** to retrieve and play the recorded audio.

---

## 📊 Example Output

```
Connecting to WiFi...
Connected! IP Address: 192.168.1.100
HTTP Server started.
Recording Start...
Sound recording 100%
File saved as /recording.wav
```

---

## 📝 Notes & Optimization Tips

- 🎵 **Recording duration:** Default is **20 seconds** but can be modified.
- 💾 **Storage:** Files are stored in **SPIFFS** and may be overwritten.
- ⚙️ **Adjustable Recording Time:** Modify `RECORD_TIME` in the code.
- 📡 **WiFi Configuration:** Ensure your ESP32 is connected to a stable WiFi network.

---

## 📜 License

This project is released under the **MIT License**.

---

## 👨‍💻 Author & Contributions

🚀 **Ibrar Ahmad**  
💡 **Embedded Engineer & PCB Designer**  
🌍 [GitHub](https://github.com/yourusername) | 📧 [Email](mailto:your.email@example.com)

---

📢 **Contributions Welcome!** Feel free to open an issue or submit a pull request.







