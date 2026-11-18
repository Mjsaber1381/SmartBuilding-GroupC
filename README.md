# SmartBuilding Group C — Week 6

ESP32 firmware and the supplied read-only dashboard for `ENV_GROUP_C_01`.
This version retains the working Week 4 wiring and Week 5 record format.

## Hardware

| Device | Bus / pins | Address |
| --- | --- | --- |
| SHT31 T1 | Wire: SDA22, SCL23 | 0x44 |
| BH1750 | Wire: SDA22, SCL23 | 0x23 |
| SHT31 T2 | Wire1: SDA18, SCL19 | 0x44 |
| PIR | GPIO13 | — |

ESP32 `esp32dev`, upload COM6 at 115200 baud, monitor COM6 at 9600 baud.
Sampling interval: 30 seconds. PIR polling: 100 ms, warm-up: 60 seconds.
The two SHT31 sensors remain on separate buses because both use address 0x44.

## Firmware setup

Open `firmware/SmartBuilding_GroupC` in PlatformIO. Copy
`include/secrets.example.h` to `include/secrets.h` on a new checkout.
Set `WIFI_SSID`, `WIFI_PASSWORD` and `DEVICE_TOKEN` there. Use a permitted
2.4 GHz network; captive portals are not implemented. The device token and
Wi-Fi configuration are excluded from Git. The existing local `secrets.h`
already contains the device token from the private `C/.env` file; Wi-Fi
profiles for home and the lab are configured locally. The example header
contains blank fields for a new checkout; actual credentials are not published.

Multiple locations are supported in the same `secrets.h`: set `WIFI_SSID`
and `WIFI_PASSWORD` for the preferred network, then fill in additional
SSID/password pairs in `WIFI_NETWORKS` for the lab, studio or a hotspot.
Add more pairs to that list as needed. Empty names are skipped. The ESP32
tries each configured network for 15 seconds, repeats the list when none
connects, and stays on a connected network. After a disconnection it starts
with the preferred network again. Rebuild and upload after changing the
list; once both locations are configured, moving between them needs no
further firmware changes. Existing single-network configs remain compatible.

Build and upload firmware:

```text
pio run
pio run -t upload
pio device monitor
```

LittleFS must be provisioned once on a new device with `pio run -t uploadfs`.
This replaces the filesystem and erases any queued records, so do not repeat
it during normal firmware updates. Export/back up existing flash before
provisioning a device that may contain data. Firmware mounts with
`LittleFS.begin(false)` and never formats automatically.

Records are created only after an actual NTP synchronization callback.
Before that, local sensor readings are printed but no UTC records are made.
Each record is saved through a temporary file, read-back verification and
rename before it can be sent. A separate network task prevents HTTPS waits
from blocking normal PIR polling. Retries reuse the saved payload unchanged.
Queue filenames encode all 128 UUID bits in a 22-character URL-safe form;
the full `record_id` in the JSON remains unchanged. This keeps temporary
filenames within the supplied filesystem image tool's 32-byte name limit.
The active queue holds up to 2000 records, subject to actual flash capacity;
existing unacknowledged data is never discarded to make room.

HTTPS validates the server certificate using the bundled GTS Root R4 CA.
The root certificate is public; it is not a credential. Check the server's
certificate chain again if the endpoint changes. The firmware does not use
`setInsecure()`.

Only HTTP 200 with valid JSON, `success: true`, the correct `device_id`, and
`accepted: 1` removes a record. Permanent rejections are moved to
`/quarantine` for diagnosis, retaining the original record. Transient
failures stay queued. Retry attempts are spaced by at least five seconds.
Sample variable names are retained where their roles match; the sample
enum `DISABLED` is named `TOKEN_MISSING` to avoid the ESP32 GPIO macro.

## Dashboard

GitHub Pages publishing source: branch `main`, folder `/docs`.
Dashboard URL: https://Mjsaber1381.github.io/SmartBuilding-GroupC/

The dashboard calls the instructor's `read-group-data` endpoint. Enter the
separate dashboard access code in the page; it remains in memory for that
tab. Never enter or embed the ESP32 device token in dashboard source.
Ask the instructor to allow the origin `https://Mjsaber1381.github.io` in
the read endpoint's CORS configuration. Without the dashboard code and
allowed origin, hosting can work while live data access remains unavailable.

The dashboard displays T1/T2, RH1/RH2, signed differences T2−T1 and RH2−RH1,
illuminance, interval motion, recorded/received timestamps and freshness.
Differences are calculated in the browser and are not added to the database.

## Current validation status

Firmware and LittleFS have been uploaded on COM6 after a verified full-flash
backup. The current location's Wi-Fi connection, real NTP synchronization,
both SHT31 sensors, BH1750, and valid PIR reporting after warm-up were checked.
The final short check produced three real records, each saved locally,
acknowledged over verified HTTPS, and removed from the queue after ACK.
The lab Wi-Fi profile is configured but has not been checked at that location.

The dashboard is published, but live browser access remains blocked by the
read API's HTTP 403 `Origin not allowed` response. The TA must allow
`https://mjsaber1381.github.io` and privately provide the Group C dashboard
access code. See `docs/TA_CONNECTION.md` for the handoff details.

The 30-minute run, deliberate outage/restart experiments and scientific
report remain pending. Extended laboratory tests were deferred at the
owner's request; the short deployment check does not establish all of the
assignment's experimental acceptance criteria.

See `docs/week06_group_c_schema_contract.md` for the data contract and
`docs/logbook.md` for the implementation record.
