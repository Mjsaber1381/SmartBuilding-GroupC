#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "cloud_client.h"
#include "server_ca.h"

static constexpr char DEVICE_ID[] = "ENV_GROUP_C_01";
static constexpr char INGEST_URL[] = "https://izbtxfmkfmkfledrqtpu.supabase.co/functions/v1/ingest-device-data";

SendResult cloudSend(const String& record) {
  if (WiFi.status() != WL_CONNECTED) return SendResult::RETRY;
  if (strlen(DEVICE_TOKEN) < 12 || String(DEVICE_TOKEN).startsWith("REPLACE_")) {
    Serial.println("SEND_DISABLED: set device token in secrets.h"); return SendResult::TOKEN_MISSING;
  }
  JsonDocument rec;
  if (deserializeJson(rec, record) || String(rec["type"] | "") != "group_c" ||
      !rec["record_id"].is<const char*>() || !rec["recorded_at"].is<const char*>())
    return SendResult::PERMANENT;
  JsonDocument body; body["device_id"] = DEVICE_ID;
  JsonArray arr = body["records"].to<JsonArray>();
  arr.add(rec.as<JsonObject>());
  String json; serializeJson(body, json);
  WiFiClientSecure tls;
  tls.setCACert(ROOT_CA);
  tls.setHandshakeTimeout(5);
  HTTPClient http;
  if (!http.begin(tls, INGEST_URL)) return SendResult::RETRY;
  http.setTimeout(4500);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-device-id", DEVICE_ID);
  http.addHeader("x-device-token", DEVICE_TOKEN);
  int status = http.POST(json);
  String response = http.getString();
  http.end();
  Serial.printf("HTTP_STATUS %d record=%s\n", status, rec["record_id"].as<const char*>());
  if (status == 400 || status == 401 || status == 403 || status == 405 || status == 413 || status == 422)
    return SendResult::PERMANENT;
  if (status != 200) return SendResult::RETRY;
  JsonDocument ack;
  if (deserializeJson(ack, response)) return SendResult::RETRY;
  bool accepted = (ack["success"] | false) &&
      String(ack["device_id"] | "") == DEVICE_ID && (ack["accepted"] | -1) == 1;
  return accepted ? SendResult::ACK : SendResult::RETRY;
}
