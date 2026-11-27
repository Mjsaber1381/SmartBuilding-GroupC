# Week 6 implementation log

## 2026-11-18

- Created a separate Week 6 working project; Week 4 and the Week 5 backup remain available.
- Reused working two-bus wiring, COM6 and 9600-baud serial output.
- Preserved the sample dashboard and compatible variable names.
- Added an editable ignored secrets header, loading only the local device token from C/.env.
- Integrated actual NTP gating, durable LittleFS storage and a network worker.
- Corrected full LittleFS file paths used during recovery, reading and deletion.
- Preserved rejected records in quarantine and required complete server ACKs before deletion.
- Replaced insecure TLS with the public GTS Root R4 trust anchor.
- Firmware build succeeded: RAM 28164/327680 bytes, flash 669809/1310720 bytes.
- LittleFS image build succeeded, containing the empty queue and quarantine directories.
- JavaScript syntax check passed.
- Pending: Wi-Fi configuration, flash backup/provisioning/upload, actual cloud ACKs and live dashboard access.
- Extended laboratory tests and scientific evidence collection were deferred by the owner.
- Full source pushed to the public repository https://github.com/Mjsaber1381/SmartBuilding-GroupC using Git authenticated as Mjsaber1381.
- GitHub Pages configured through the GitHub API: branch main, directory /docs, HTTPS enforced. Updates to the publishing source trigger deployment.

## Hardware deployment, 2026-11-18

- Rebuilt with the owner's private home and lab Wi-Fi profiles.
- Saved a verified 4 MB pre-deployment flash backup locally. The filesystem partition was erased/unused, and the new partition layout matched.
- Provisioned LittleFS once, then uploaded firmware on COM6 at 115200 baud.
- The first live check found that the supplied image tool's 32-byte filename limit rejected UUID filenames with `.json.tmp`. Queue filenames now encode the complete 128-bit UUID in 22 URL-safe characters. JSON UUIDs and timestamps remain unchanged; legacy filenames remain recognized.
- Replaced noisy Arduino `exists()` calls with quiet filesystem checks: synchronous missing-file log messages were delaying PIR polling and invalidating its sampling window.
- Final firmware build: RAM 47708/327680 bytes; flash 1018209/1310720 bytes.
- Final 101-second check: both SHT31 sensors and BH1750 initialized, current Wi-Fi connected, NTP callback received, three records saved and acknowledged with HTTP 200, and all three queue files removed after valid ACK. PIR was null during warm-up and valid false in the first fully observed post-warm-up interval.
- Real serial output and delivery evidence are kept locally in `C/Week06_Exports/Live_2026-11-18`; credentials, compiled firmware and full-flash backups are excluded from the repository.
- Dashboard read preflight returned HTTP 403 with `{"error":"Origin not allowed"}` for `https://mjsaber1381.github.io`. Browser data access is not verified; TA origin approval and a separate dashboard access code are still required.
- Lab Wi-Fi at the lab, deliberate offline/restart experiments and the 30-minute integrated test remain unverified.

## Dashboard origin approval, 2026-11-18

- Rechecked after the TA's server update: the read API now returns HTTP 204 for the Pages-origin preflight, with the exact allowed origin, POST/OPTIONS methods and required headers.
- The separate Group C dashboard access code is still needed to verify authenticated reads.

## Authenticated dashboard connection, 2026-11-18

- Owner added the dashboard code to the private `C/.env` as `DASHBOARD_KEY_C`; the code was not written to dashboard source or committed.
- Both authenticated read actions, `latest` and `recent`, returned HTTP 200 with the correct device ID and allowed origin.
- The API returned 60 recent records. A new ESP32 record and its valid ingestion ACK were captured without resetting or reflashing the board.
- Matched the fresh record's UUID, UTC timestamp, both temperature/RH pairs, illuminance, PIR and uptime exactly against the read response. Latest-record age was eight seconds during verification.
- End-to-end acquisition, durable save, ingestion acknowledgment and authenticated dashboard data retrieval are verified. Interactive browser rendering was not controlled during this check; extended lab experiments remain pending as previously recorded.

## Dashboard redesign 2026-11-27

- Reworked the dashboard into a light environmental monitoring interface with paired-point comparison, shared light/motion conditions, all six original trends, and a six-row recent-readings table.
- Self-hosted the supplied Kode Mono variable font with its SIL Open Font License; retained the existing buildless GitHub Pages deployment and read API.
- Checked actual authenticated data on desktop 1440 px and mobile 390 px, error/empty/loading states, disconnect/reset, chart gaps and absence of horizontal overflow. Access-code input clears after login; no localStorage or sessionStorage is used.
- Controlled hardware outage/recovery and continuous online validation are being recorded separately on November 27 for the Week 5 and Week 6 reports.
