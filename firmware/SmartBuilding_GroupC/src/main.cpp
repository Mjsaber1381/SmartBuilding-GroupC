#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <math.h>
#include <atomic>
#include <esp_system.h>
#include <esp_sntp.h>
#include <ArduinoJson.h>
#include <Adafruit_SHT31.h>
#include <BH1750.h>
#include "secrets.h"
#include "queue_manager.h"
#include "cloud_client.h"

// Week 6: persist every timestamped record before HTTPS delivery.
constexpr char DEVICE_ID[] = "ENV_GROUP_C_01";
constexpr char RECORD_TYPE[] = "group_c";
// Keep the working Week 4 wiring. The two 0x44 sensors use separate buses.
constexpr uint8_t SDA_PIN = 22, SCL_PIN = 23;
constexpr uint8_t SHT2_SDA_PIN = 18, SHT2_SCL_PIN = 19;
constexpr uint8_t PIR_PIN = 13;
constexpr uint8_t SHT1_ADDR = 0x44, SHT2_ADDR = 0x44, BH_ADDR = 0x23;
constexpr uint32_t I2C_CLOCK_HZ = 100000;
constexpr uint32_t RECORD_INTERVAL_MS = 30000;
constexpr uint32_t PIR_POLL_MS = 100;
constexpr uint32_t PIR_WARMUP_MS = 60000;
constexpr uint32_t WIFI_RETRY_MS = 15000;
constexpr uint32_t SENSOR_RETRY_MS = 30000;
constexpr uint32_t SERIAL_KEEPALIVE_MS = 1000;
constexpr uint32_t SEND_RETRY_MS = 5000;

Adafruit_SHT31 sht1;
Adafruit_SHT31 sht2(&Wire1);
BH1750 lightMeter;
bool busReady = false, sht2BusReady = false;
bool sht1Ready = false, sht2Ready = false, lightReady = false;
bool pirCurrent = false, pirAnyHigh = false, pirWindowValid = false;
bool pirWarmedUp = false;
uint32_t lastPirPoll = 0, lastRecord = 0, lastWiFiAttempt = 0;
uint32_t lastSensorAttempt = 0;
uint32_t pirWarmupStart = 0;
uint32_t lastSerialKeepalive = 0;
std::atomic<bool> clockEverSynchronized{false};

String generateUUID() {
  uint8_t b[16];
  esp_fill_random(b, sizeof(b));
  b[6] = (b[6] & 0x0f) | 0x40;
  b[8] = (b[8] & 0x3f) | 0x80;
  char out[37];
  snprintf(out, sizeof(out),
    "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
    b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],
    b[10],b[11],b[12],b[13],b[14],b[15]);
  return String(out);
}

void onTimeSync(struct timeval*) {
  clockEverSynchronized = true;
  Serial.println("STATUS: NTP synchronization completed");
}

String getUTCTimestamp() {
  if (!clockEverSynchronized) return "";
  time_t now = time(nullptr);
  if (now < 1704067200) return "";
  struct tm utc;
  if (gmtime_r(&now, &utc) == nullptr) return "";
  char buffer[25];
  if (!strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc)) return "";
  return String(buffer);
}

void scanI2C() {
  Serial.println("I2C SCAN BEGIN");
  for (int pass = 1; pass <= 3; ++pass) {
    for (int bus = 0; bus < 2; ++bus) {
      TwoWire& wire = bus == 0 ? Wire : Wire1;
      Serial.printf("I2C scan %d | bus %d:", pass, bus);
      if (!(bus == 0 ? busReady : sht2BusReady)) {
        Serial.println(" BUS FAILED");
        continue;
      }
      for (uint8_t addr = 1; addr < 127; ++addr) {
        wire.beginTransmission(addr);
        if (wire.endTransmission() == 0) Serial.printf(" 0x%02X", addr);
      }
      Serial.println();
    }
    delay(150);
  }
  Serial.println("Expected bus 0 (SDA22/SCL23): 0x23 BH1750, 0x44 SHT31-T1");
  Serial.println("Expected bus 1 (SDA18/SCL19): 0x44 SHT31-T2");
}

void initializeSensors() {
  if (!sht1Ready) sht1Ready = busReady && sht1.begin(SHT1_ADDR);
  if (!sht2Ready) sht2Ready = sht2BusReady && sht2.begin(SHT2_ADDR);
  if (!lightReady) lightReady = busReady && lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH_ADDR, &Wire);
  Serial.printf("SENSORS SHT31_T1=%s SHT31_T2=%s BH1750=%s\n",
    sht1Ready ? "OK" : "FAIL", sht2Ready ? "OK" : "FAIL", lightReady ? "OK" : "FAIL");
}

void maintainWiFi() {
  if (WIFI_SSID[0] == '\0') return;
  if (WiFi.status() == WL_CONNECTED) return;
  uint32_t now = millis();
  if (now - lastWiFiAttempt >= WIFI_RETRY_MS) {
    lastWiFiAttempt = now;
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("STATUS: WiFi reconnect requested");
  }
}

void updatePIR() {
  uint32_t now = millis();
  if (now - lastPirPoll < PIR_POLL_MS) return;
  // If polling was blocked, the full interval cannot be verified.
  if (lastPirPoll != 0 && now - lastPirPoll > 3 * PIR_POLL_MS) pirWindowValid = false;
  lastPirPoll = now;
  pirCurrent = digitalRead(PIR_PIN) == HIGH;
  // Latch completion so millis() rollover cannot restart the warm-up.
  if (!pirWarmedUp && now - pirWarmupStart >= PIR_WARMUP_MS) pirWarmedUp = true;
  if (!pirWarmedUp) {
    pirWindowValid = false;
    return;
  }
  if (pirCurrent) pirAnyHigh = true;
}

struct SensorData {
  float t1 = NAN, rh1 = NAN, t2 = NAN, rh2 = NAN, lux = NAN;
  bool t1Valid = false, rh1Valid = false, t2Valid = false, rh2Valid = false, luxValid = false;
  bool deltaTValid = false, deltaRHValid = false;
  float deltaT = NAN, deltaRH = NAN;
};

SensorData readSensors() {
  SensorData d;
  if (sht1Ready) { d.t1 = sht1.readTemperature(); d.rh1 = sht1.readHumidity(); }
  if (sht2Ready) { d.t2 = sht2.readTemperature(); d.rh2 = sht2.readHumidity(); }
  if (lightReady) d.lux = lightMeter.readLightLevel();
  d.t1Valid = isfinite(d.t1) && d.t1 >= -50 && d.t1 <= 100;
  d.t2Valid = isfinite(d.t2) && d.t2 >= -50 && d.t2 <= 100;
  d.rh1Valid = isfinite(d.rh1) && d.rh1 >= 0 && d.rh1 <= 100;
  d.rh2Valid = isfinite(d.rh2) && d.rh2 >= 0 && d.rh2 <= 100;
  d.luxValid = isfinite(d.lux) && d.lux >= 0;
  d.deltaTValid = d.t1Valid && d.t2Valid;
  d.deltaRHValid = d.rh1Valid && d.rh2Valid;
  if (d.deltaTValid) d.deltaT = d.t2 - d.t1;
  if (d.deltaRHValid) d.deltaRH = d.rh2 - d.rh1;
  // Reinitialize disconnected devices on a later retry, preserving per-channel validity.
  if (!d.t1Valid && !d.rh1Valid) sht1Ready = false;
  if (!d.t2Valid && !d.rh2Valid) sht2Ready = false;
  if (!d.luxValid) lightReady = false;
  return d;
}

void setOptionalNumber(JsonObject obj, const char* key, float value, bool valid) {
  if (valid) obj[key] = value;
  else obj[key] = nullptr;
}

void printGroupCRecord(const String& timestamp, const SensorData& d,
                       bool pirAnyHighForRecord, bool pirValidForRecord) {
  JsonDocument doc;
  JsonObject record = doc.to<JsonObject>();
  record["type"] = RECORD_TYPE;
  record["record_id"] = generateUUID();
  record["recorded_at"] = timestamp;
  setOptionalNumber(record, "temperature_1_c", d.t1, d.t1Valid);
  setOptionalNumber(record, "humidity_1_rh", d.rh1, d.rh1Valid);
  setOptionalNumber(record, "temperature_2_c", d.t2, d.t2Valid);
  setOptionalNumber(record, "humidity_2_rh", d.rh2, d.rh2Valid);
  setOptionalNumber(record, "illuminance_lux", d.lux, d.luxValid);
  if (pirValidForRecord) record["motion_detected"] = pirAnyHighForRecord;
  else record["motion_detected"] = nullptr;
  record["device_uptime_ms"] = static_cast<uint64_t>(millis());
  String payload;
  serializeJson(doc, payload);
  const String id = record["record_id"].as<String>();
  if (!queueHealthy() || !queueSave(id, payload)) {
    Serial.println("CRITICAL_STORAGE_FAILURE: record was not durably saved");
  } else {
    Serial.printf("QUEUED %s pending=%u\n", id.c_str(), (unsigned)queueCount());
  }
  Serial.print("GROUP_C_RECORD: ");
  Serial.print(payload);
  Serial.println();
}

void printSpatialAnalysis(const SensorData& d) {
  Serial.print("ANALYSIS_ONLY: delta_T_C=");
  if (d.deltaTValid) Serial.print(d.deltaT, 3); else Serial.print("null");
  Serial.print(" delta_RH_pp=");
  if (d.deltaRHValid) Serial.println(d.deltaRH, 3); else Serial.println("null");
}

void networkWorker(void*) {
  uint32_t lastSend = 0;
  for (;;) {
    maintainWiFi();
    const uint32_t now = millis();
    if (clockEverSynchronized && WiFi.status() == WL_CONNECTED &&
        queueHealthy() && now - lastSend >= SEND_RETRY_MS) {
      lastSend = now;
      String path, payload;
      if (queuePeek(path, payload)) {
        const SendResult result = cloudSend(payload);
        if (result == SendResult::ACK) {
          if (queueRemove(path)) Serial.println("ACK_REMOVED " + path);
          else Serial.println("CRITICAL_ACK_DELETE_FAILED " + path);
        } else if (result == SendResult::PERMANENT) {
          if (!queueReject(path)) Serial.println("CRITICAL_QUARANTINE_FAILED " + path);
          else Serial.println("SERVER_REJECTED_QUARANTINED " + path);
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {
  // Buffer a complete JSON + local analysis at 9600 baud without blocking PIR polling.
  Serial.setTxBufferSize(2048);
  Serial.begin(9600);
  delay(300);
  Serial.println("SmartBuilding Group C | Week 06 | DURABLE QUEUE + VERIFIED HTTPS");
  pinMode(PIR_PIN, INPUT);
  pirWarmupStart = millis();
  busReady = Wire.begin(SDA_PIN, SCL_PIN, I2C_CLOCK_HZ);
  sht2BusReady = Wire1.begin(SHT2_SDA_PIN, SHT2_SCL_PIN, I2C_CLOCK_HZ);
  Wire.setTimeOut(25);
  Wire1.setTimeOut(25);
  scanI2C();
  initializeSensors();
  Serial.printf("PIR GPIO%u | warm-up %lu ms | record interval %lu ms\n",
    PIR_PIN, static_cast<unsigned long>(PIR_WARMUP_MS),
    static_cast<unsigned long>(RECORD_INTERVAL_MS));
  if (!queueInit()) Serial.println("CRITICAL: LittleFS mount failed; provision data with uploadfs before use");
  else { queueRecover(); Serial.printf("RECOVERED_QUEUE_COUNT %u\n", (unsigned)queueCount()); }
  if (WIFI_SSID[0] != '\0') {
    WiFi.mode(WIFI_STA);
    sntp_set_time_sync_notification_cb(onTimeSync);
    configTime(0, 0, "pool.ntp.org", "time.google.com");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastWiFiAttempt = millis();
    Serial.println("STATUS: WiFi connection requested; awaiting NTP");
  } else {
    Serial.println("STATUS: WiFi not configured; edit include/secrets.h and upload again");
  }
  lastRecord = millis();
  lastPirPoll = millis();
  lastSensorAttempt = millis();
  if (xTaskCreatePinnedToCore(networkWorker, "cloudWorker", 10000, nullptr, 1, nullptr, 0) != pdPASS)
    Serial.println("CRITICAL: cloud worker could not start");
}

void updateRecords() {
  uint32_t now = millis();
  if (now - lastRecord < RECORD_INTERVAL_MS) return;
  // Start the next real interval here; do not emit catch-up records after a stall.
  lastRecord = now;
  const bool pirAnyHighForRecord = pirAnyHigh;
  const bool pirValidForRecord = pirWindowValid && pirWarmedUp;
  pirAnyHigh = false;
  pirWindowValid = true;
  SensorData d = readSensors();
  // Local readings remain available before Wi-Fi/NTP has been configured.
  Serial.printf("LOCAL_SAMPLE: Uptime_ms=%lu T1_C=%.2f RH1_pct=%.2f T2_C=%.2f RH2_pct=%.2f Lux=%.2f PIR_Current=%d PIR_AnyHigh=%d PIR_Valid=%d\n",
    static_cast<unsigned long>(now), d.t1Valid ? d.t1 : NAN,
    d.rh1Valid ? d.rh1 : NAN, d.t2Valid ? d.t2 : NAN,
    d.rh2Valid ? d.rh2 : NAN, d.luxValid ? d.lux : NAN,
    pirCurrent ? 1 : 0, pirValidForRecord ? (pirAnyHighForRecord ? 1 : 0) : -1,
    pirValidForRecord ? 1 : 0);
  printSpatialAnalysis(d);
  const String timestamp = getUTCTimestamp();
  if (timestamp.isEmpty()) {
    Serial.println("SKIP: No verified NTP synchronization; no fabricated UTC record");
    return;
  }
  printGroupCRecord(timestamp, d, pirAnyHighForRecord, pirValidForRecord);
}

void loop() {
  updatePIR();
  uint32_t now = millis();
  if (now - lastSensorAttempt >= SENSOR_RETRY_MS) {
    lastSensorAttempt = now;
    if (!sht1Ready || !sht2Ready || !lightReady) initializeSensors();
  }
  updatePIR();
  updateRecords();
  if (now - lastSerialKeepalive >= SERIAL_KEEPALIVE_MS) {
    lastSerialKeepalive = now;
    Serial.println();
  }
  delay(2);
}
