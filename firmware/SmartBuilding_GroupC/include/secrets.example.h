#pragma once
#include "wifi_config.h"

// First/preferred network, e.g. home. Keep the sample variable names.
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define DEVICE_TOKEN ""

// Add more entries as needed. Empty network names are skipped.
#define WIFI_NETWORKS_CONFIGURED
static const WiFiCredentials WIFI_NETWORKS[] = {
  {WIFI_SSID, WIFI_PASSWORD}, // Home
  {"", ""},                 // Lab / studio
};
