# Group C dashboard connection

Device: `ENV_GROUP_C_01`

Dashboard: https://mjsaber1381.github.io/SmartBuilding-GroupC/

Origin to allow: `https://mjsaber1381.github.io`

The ESP32 is running Week 6 firmware with the working two-bus sensor wiring.
The final short hardware check verified three real records saved to LittleFS,
accepted by `ingest-device-data` with valid HTTP 200 acknowledgments, and then
removed from the local queue. NTP synchronization and PIR after warm-up passed.

The dashboard uses the read-only `read-group-data` Edge Function. Its preflight
request currently returns HTTP 403 with `{"error":"Origin not allowed"}`.

TA-side setup needed:

1. Allow `https://mjsaber1381.github.io` in the read function's origin/CORS configuration.
2. Privately provide the Group C dashboard access code for the `x-dashboard-key` header.

The ESP32 device token is already configured privately and must not be placed
in dashboard code. Live browser data access remains to be verified after the
origin and access code are available.
