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
