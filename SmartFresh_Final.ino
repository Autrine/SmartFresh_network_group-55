/*
 * SmartFresh - Final Full System Code
 * ESP32-CAM + SHTC3 + Edge Impulse + Firebase
 *
 * Classes: fresh, raw, spoilt
 *
 * Wiring:
 *   SHTC3 SDA --> GPIO 14
 *   SHTC3 SCL --> GPIO 15
 *   SHTC3 VCC --> 3.3V
 *   SHTC3 GND --> GND
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <SFresh_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_camera.h"
#include "Adafruit_SHTC3.h"
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ══════════════════════════════════════════════════════════════════════════════
//  YOUR CONFIGURATION
// ══════════════════════════════════════════════════════════════════════════════
#define WIFI_SSID        "BIGALA"
#define WIFI_PASSWORD    "@housebigala"

#define FIREBASE_API_KEY      "AIzaSyC-2ZDwV9atllW37ptaCg61BvhQEV_31Tg"
#define FIREBASE_DATABASE_URL "https://sfresh-3e57d-default-rtdb.firebaseio.com"

#define I2C_SDA 14
#define I2C_SCL 15

// ══════════════════════════════════════════════════════════════════════════════
//  Camera pins
// ══════════════════════════════════════════════════════════════════════════════
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define EI_CAMERA_RAW_FRAME_BUFFER_COLS  320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS  240
#define EI_CAMERA_FRAME_BYTE_SIZE        3

static bool  is_initialised = false;
static bool  debug_nn       = false;
uint8_t     *snapshot_buf;

static camera_config_t camera_config = {
  .pin_pwdn     = PWDN_GPIO_NUM,
  .pin_reset    = RESET_GPIO_NUM,
  .pin_xclk     = XCLK_GPIO_NUM,
  .pin_sscb_sda = SIOD_GPIO_NUM,
  .pin_sscb_scl = SIOC_GPIO_NUM,
  .pin_d7 = Y9_GPIO_NUM, .pin_d6 = Y8_GPIO_NUM,
  .pin_d5 = Y7_GPIO_NUM, .pin_d4 = Y6_GPIO_NUM,
  .pin_d3 = Y5_GPIO_NUM, .pin_d2 = Y4_GPIO_NUM,
  .pin_d1 = Y3_GPIO_NUM, .pin_d0 = Y2_GPIO_NUM,
  .pin_vsync    = VSYNC_GPIO_NUM,
  .pin_href     = HREF_GPIO_NUM,
  .pin_pclk     = PCLK_GPIO_NUM,
  .xclk_freq_hz = 20000000,
  .ledc_timer   = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  .pixel_format = PIXFORMAT_JPEG,
  .frame_size   = FRAMESIZE_QVGA,
  .jpeg_quality = 12,
  .fb_count     = 1,
  .fb_location  = CAMERA_FB_IN_PSRAM,
  .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
};

FirebaseData   fbdo;
FirebaseData   fbdo_stream;
FirebaseAuth   auth;
FirebaseConfig config;
Adafruit_SHTC3 shtc3;

bool captureRequested = false;
bool firebaseReady    = false;

// ══════════════════════════════════════════════════════════════════════════════
//  Freshness info — fresh, raw, spoilt
// ══════════════════════════════════════════════════════════════════════════════
struct FreshnessInfo {
  String shelfLife;
  String recommendation;
  String conditionLevel;
};

FreshnessInfo getFreshnessInfo(const String &label) {
  if (label == "fresh")  return {"4-6 days",      "Continue selling. Tomatoes are ripe and in good condition.", "Good"};
  if (label == "raw")    return {"Not ready yet",  "Do not sell yet. Tomatoes are not ripe. Store and wait.",   "Not Ripe"};
  if (label == "spoilt") return {"0 days",         "Remove from sale immediately. Tomatoes are spoilt.",        "Bad"};
  return                        {"--",             "No result yet.", "--"};
}

// ══════════════════════════════════════════════════════════════════════════════
//  Camera functions
// ══════════════════════════════════════════════════════════════════════════════
bool ei_camera_init(void) {
  if (is_initialised) return true;
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) { Serial.printf("[Camera] Init failed: 0x%x\n", err); return false; }
  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) { s->set_vflip(s,1); s->set_brightness(s,1); s->set_saturation(s,0); }
  is_initialised = true;
  Serial.println("[Camera] Ready");
  return true;
}

bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
  if (!is_initialised) return false;
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { Serial.println("[Camera] Capture failed"); return false; }
  bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
  esp_camera_fb_return(fb);
  if (!converted) { Serial.println("[Camera] Conversion failed"); return false; }
  if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS) || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
    ei::image::processing::crop_and_interpolate_rgb888(out_buf, EI_CAMERA_RAW_FRAME_BUFFER_COLS, EI_CAMERA_RAW_FRAME_BUFFER_ROWS, out_buf, img_width, img_height);
  }
  return true;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
  size_t pixel_ix = offset * 3, pixels_left = length, out_ptr_ix = 0;
  while (pixels_left != 0) {
    out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix+2] << 16) + (snapshot_buf[pixel_ix+1] << 8) + snapshot_buf[pixel_ix];
    out_ptr_ix++; pixel_ix += 3; pixels_left--;
  }
  return 0;
}

// ══════════════════════════════════════════════════════════════════════════════
//  Run inference
// ══════════════════════════════════════════════════════════════════════════════
bool runInference(String &outLabel, float &outConfidence) {
  snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
  if (!snapshot_buf) { Serial.println("[EI] Not enough RAM"); return false; }

  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data     = &ei_camera_get_data;

  if (!ei_camera_capture(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf)) {
    free(snapshot_buf); return false;
  }

  ei_impulse_result_t result = {0};
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
  if (err != EI_IMPULSE_OK) { Serial.printf("[EI] Error: %d\n", err); free(snapshot_buf); return false; }

  float bestScore = -1; int bestIdx = 0;
  Serial.println("[EI] Predictions:");
  for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    Serial.printf("  %s: %.5f\n", ei_classifier_inferencing_categories[i], result.classification[i].value);
    if (result.classification[i].value > bestScore) { bestScore = result.classification[i].value; bestIdx = i; }
  }

  outLabel      = String(ei_classifier_inferencing_categories[bestIdx]);
  outConfidence = bestScore * 100.0f;
  Serial.printf("[EI] Best: %s (%.1f%%)\n", outLabel.c_str(), outConfidence);
  free(snapshot_buf);
  return true;
}

// ══════════════════════════════════════════════════════════════════════════════
//  Firebase stream callback
// ══════════════════════════════════════════════════════════════════════════════
void streamCallback(FirebaseStream data) {
  if (data.dataPath() == "/capture" && data.boolData() == true) {
    captureRequested = true;
    Serial.println("[Firebase] Capture command received!");
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) Serial.println("[Firebase] Stream timeout — reconnecting...");
}

// ══════════════════════════════════════════════════════════════════════════════
//  Send results to Firebase
// ══════════════════════════════════════════════════════════════════════════════
void sendResults(const String &label, float confidence, float temp, float hum, const FreshnessInfo &info) {
  FirebaseJson json;
  json.set("label",          label);
  json.set("confidence",     confidence);
  json.set("temperature",    temp);
  json.set("humidity",       hum);
  json.set("shelfLife",      info.shelfLife);
  json.set("recommendation", info.recommendation);
  json.set("conditionLevel", info.conditionLevel);
  json.set("timestamp",      (int)(millis() / 1000));

  if (Firebase.RTDB.setJSON(&fbdo, "/latest", &json)) {
    Serial.println("[Firebase] /latest updated");
  } else {
    Serial.println("[Firebase] Error: " + fbdo.errorReason());
  }

  String histPath = "/readings/" + String(millis());
  Firebase.RTDB.setJSON(&fbdo, histPath.c_str(), &json);
  Firebase.RTDB.setBool(&fbdo, "/command/capture", false);
}

// ══════════════════════════════════════════════════════════════════════════════
//  Full capture cycle
// ══════════════════════════════════════════════════════════════════════════════
void doCapture() {
  Serial.println("\n[SmartFresh] Starting capture...");
  Firebase.RTDB.setString(&fbdo, "/status", "scanning");

  sensors_event_t hum_ev, temp_ev;
  shtc3.getEvent(&hum_ev, &temp_ev);
  float temperature = temp_ev.temperature;
  float humidity    = hum_ev.relative_humidity;
  Serial.printf("[SHTC3] Temp: %.1f C  Hum: %.1f%%\n", temperature, humidity);

  String label; float confidence;
  if (!runInference(label, confidence)) {
    Serial.println("[SmartFresh] Inference failed");
    Firebase.RTDB.setString(&fbdo, "/status", "error");
    Firebase.RTDB.setBool(&fbdo, "/command/capture", false);
    return;
  }

  FreshnessInfo info = getFreshnessInfo(label);

  Serial.println("[SmartFresh] Result:");
  Serial.println("  Label:      " + label);
  Serial.printf( "  Confidence: %.1f%%\n", confidence);
  Serial.printf( "  Temp:       %.1f C\n", temperature);
  Serial.printf( "  Humidity:   %.1f%%\n", humidity);
  Serial.println("  Shelf life: " + info.shelfLife);
  Serial.println("  Action:     " + info.recommendation);

  sendResults(label, confidence, temperature, humidity, info);
  Firebase.RTDB.setString(&fbdo, "/status", "ready");
  Serial.println("[SmartFresh] Done!\n");
}

// ══════════════════════════════════════════════════════════════════════════════
//  SETUP
// ══════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== SmartFresh Final System ===");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n[WiFi] " + WiFi.localIP().toString());

  Wire.begin(I2C_SDA, I2C_SCL);
  if (!shtc3.begin()) { Serial.println("[SHTC3] Not found!"); while (1) delay(1000); }
  Serial.println("[SHTC3] Ready");

  if (!ei_camera_init()) { Serial.println("[Camera] Failed!"); while (1) delay(1000); }

  config.api_key      = FIREBASE_API_KEY;
  config.database_url = FIREBASE_DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  Firebase.signUp(&config, &auth, "", "");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.print("[Firebase] Connecting");
  unsigned long t = millis();
  while (!Firebase.ready()) {
    if (millis() - t > 15000) { Serial.println("\n[Firebase] Timeout!"); break; }
    delay(500); Serial.print(".");
  }

  if (Firebase.ready()) { firebaseReady = true; Serial.println("\n[Firebase] Ready!"); }

  if (!Firebase.RTDB.beginStream(&fbdo_stream, "/command")) {
    Serial.println("[Firebase] Stream error: " + fbdo_stream.errorReason());
  } else {
    Firebase.RTDB.setStreamCallback(&fbdo_stream, streamCallback, streamTimeoutCallback);
    Serial.println("[Firebase] Listening for dashboard commands...");
  }

  Firebase.RTDB.setString(&fbdo, "/status", "ready");
  Firebase.RTDB.setBool(&fbdo,   "/command/capture", false);

  Serial.println("\n=== Ready! Press Capture Image on your dashboard ===\n");
}

// ══════════════════════════════════════════════════════════════════════════════
//  LOOP
// ══════════════════════════════════════════════════════════════════════════════
void loop() {
  if (captureRequested) { captureRequested = false; doCapture(); }
  if (Firebase.isTokenExpired()) Firebase.refreshToken(&config);
  delay(100);
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "Invalid model for current sensor"
#endif
