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
