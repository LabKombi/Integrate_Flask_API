#include <WiFi.h>
#include <LittleFS.h>
#include <esp_camera.h>
#include <HTTPClient.h>
#include <FS.h>

#define WIFI_SSID "YOUR_WIFI_SSID"  // Ganti dengan SSID WiFi Anda
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"  // Ganti dengan Password WiFi Anda

#define FILE_PHOTO_PATH "/photo.jpg"
#define API_URL "http://<YOUR_SERVER_IP>:5000/process_image"  // Ganti dengan alamat IP server Flask Anda

unsigned long previousMillis = 0;  
const long interval = 1000;  // Interval 1 detik (1000 milidetik)

// Definisi Pin untuk Kamera (ESP32-CAM)
#define Y2_GPIO_NUM    5   // D5
#define Y3_GPIO_NUM    18  // D18
#define Y4_GPIO_NUM    19  // D19
#define Y5_GPIO_NUM    21  // D21
#define Y6_GPIO_NUM    36  // D36
#define Y7_GPIO_NUM    39  // D39
#define Y8_GPIO_NUM    34  // D34
#define Y9_GPIO_NUM    35  // D35
#define XCLK_GPIO_NUM  0   // D0
#define PCLK_GPIO_NUM  22  // D22
#define VSYNC_GPIO_NUM 25  // D25
#define HREF_GPIO_NUM  23  // D23
#define SIOD_GPIO_NUM  26  // D26
#define SIOC_GPIO_NUM  27  // D27
#define PWDN_GPIO_NUM  32  // D32
#define RESET_GPIO_NUM -1  // Tidak digunakan

void capturePhotoSaveLittleFS() {
  camera_fb_t* fb = NULL;

  // Ambil gambar
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Simpan gambar ke LittleFS
  File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  } else {
    file.write(fb->buf, fb->len);
    Serial.println("Photo saved to LittleFS");
  }
  file.close();
  esp_camera_fb_return(fb);
}

void sendPhotoToAPI() {
  File file = LittleFS.open(FILE_PHOTO_PATH, "r");
  if (!file) {
    Serial.println("Failed to open photo file");
    return;
  }

  // Prepare HTTP client
  HTTPClient http;
  http.begin(API_URL);  // URL API Flask
  http.addHeader("Content-Type", "multipart/form-data");

  // Boundary for multipart form data
  String boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
  String body = "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"image\"; filename=\"photo.jpg\"\r\n";
  body += "Content-Type: image/jpeg\r\n\r\n";

  // Send the photo file content
  http.addHeader("Content-Length", String(file.size() + body.length() + 4));

  WiFiClient *client = http.getStreamPtr();
  client->print(body);  // Add multipart header
  
  // Send photo content
  uint8_t buffer[128];
  int bytesRead;
  while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
    client->write(buffer, bytesRead);
  }

  client->print("\r\n--" + boundary + "--\r\n");  // End of multipart form-data
  
  // Close file and send request
  file.close();
  
  int httpCode = http.POST("");  // Send the POST request
  
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Response: " + payload);
  } else {
    Serial.println("Error in HTTP request");
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    delay(1000);
  }
  Serial.print("Connected: ");
  Serial.println(WIFI_SSID);
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  if (!LittleFS.begin(true)) {
    Serial.println("An error occurred while mounting LittleFS");
    return;
  }

  // Set up camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;  // Ganti pin_sscb_sda menjadi pin_sccb_sda
  config.pin_sccb_scl = SIOC_GPIO_NUM;  // Ganti pin_sscb_scl menjadi pin_sccb_scl
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  Serial.println("Camera initialized successfully.");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Jika sudah lewat 1 detik (interval)
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Simpan waktu pengambilan gambar terakhir
    
    // Ambil gambar dan simpan ke LittleFS
    capturePhotoSaveLittleFS();
    
    // Kirim gambar ke API Flask
    sendPhotoToAPI();
  }

  // Lakukan tugas lain (misalnya melayani permintaan web) tanpa menghalangi pengambilan gambar
}
