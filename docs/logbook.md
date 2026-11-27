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

## Extended hardware validation 2026-11-27

- Used an isolated diagnostic copy to pause/resume Wi-Fi and perform a controlled restart; production firmware source was not modified and LittleFS was not formatted.
- The first planned outage ended early because the diagnostic pause flag did not survive restart. It is not counted as the 600-second pass. The restart nevertheless recovered all 19 pending records with exact cloud matches.
- A second uninterrupted outage lasted 600 seconds and generated 20 real records. All were later acknowledged and matched by UUID, timestamp and values. The highest observed backlog was 32 records; this was not a filesystem-capacity test.
- A separate uninterrupted online window lasted 1805.125 seconds and yielded 60 records, with intervals 29.998–30.002 seconds and 180 connected Wi-Fi status samples.
- Across 105 readable diagnostic records and six restored-production records, all 111 traced UUIDs had valid ACK/delete evidence and exact authenticated cloud matches. Four pre-existing queued records were also acknowledged. The raw UART has one missing/corrupted QUEUED marker and other garbled diagnostic lines; no claim is made that UART observation independently counts every possible emitted record.
- All five environmental channels were non-null in the 60-record online window. PIR was null in 32 intervals; production recheck had two valid false values and four nulls, including two after warm-up. Continuous PIR coverage remains unresolved and is explicitly reported.
- Generated 48 valid, unique hardware UUID-v4 values across two boots. A bad-auth request returned 401; replay of one unchanged real record returned 200 and the read API showed a single matching ID. Direct SQL verification was not performed.
- Recorded ten adjacent-sensor pairs at the owner's confirmed approximate 0.90 m height: mean T2−T1 +0.028 °C, temperature MAE 0.030 °C; mean RH2−RH1 +0.180 percentage points, humidity MAE 0.332 percentage points. This describes pair agreement, not absolute calibration. The owner could not separate the sensors for the second placement test.
- Restored the original production application, SHA-256 `44d0233276396f6a58606eb42eedc3e6bcec7a60173be5bf3bd69331d956f67c`; uploader hash verification and six fresh cloud ACKs passed. Partition layout and queue were preserved.
- Report-safe evidence is held locally in `C/Week06_Exports/Validation_2026-11-27`. Private diagnostic configuration and compiled binaries are excluded from public source and report delivery.
