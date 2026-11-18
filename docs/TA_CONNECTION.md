# Group C dashboard connection

Device: `ENV_GROUP_C_01`

Dashboard: https://mjsaber1381.github.io/SmartBuilding-GroupC/

Allowed origin: `https://mjsaber1381.github.io`

The ESP32 is running Week 6 firmware with the working two-bus sensor wiring.
The final short hardware check verified three real records saved to LittleFS,
accepted by `ingest-device-data` with valid HTTP 200 acknowledgments, and then
removed from the local queue. NTP synchronization and PIR after warm-up passed.

The dashboard uses the read-only `read-group-data` Edge Function. Its preflight
now returns HTTP 204 with `Access-Control-Allow-Origin` matching the site,
allowed methods `POST,OPTIONS`, and headers `content-type,x-dashboard-key`.

The Group C dashboard access code has now been provided privately. Both
authenticated `latest` and `recent` reads returned HTTP 200. A fresh record
captured from the ESP32 and acknowledged by ingestion was matched exactly
against the read API by UUID, timestamp, sensor values and uptime.

The ESP32 device token is already configured privately and must not be placed
in dashboard code. Enter the separate dashboard access code in the published
page and click **Connect**. The code remains in memory for that browser tab.
