# Week 6 Group C contract

Source: supplied Week 6 assignment and firmware/dashboard packages.
Destination: `public.group_c_data`. Device: `ENV_GROUP_C_01`. Type: `group_c`.

| Column | Type | Nullable |
| --- | --- | --- |
| record_id | text | no |
| device_id | text | no |
| recorded_at | timestamptz | no |
| received_at | timestamptz, server default now() | no |
| temperature_1_c, temperature_2_c | double precision | yes |
| humidity_1_rh, humidity_2_rh | double precision | yes |
| illuminance_lux | double precision | yes |
| motion_detected | boolean | yes |
| device_uptime_ms | bigint | yes |

POST `https://izbtxfmkfmkfledrqtpu.supabase.co/functions/v1/ingest-device-data`
with `Content-Type: application/json`, `x-device-id: ENV_GROUP_C_01` and
the private `x-device-token` header.

The body is `{"device_id":"ENV_GROUP_C_01","records":[record]}`.
The firmware sends one record per request; the contract permits 1–10.
Each record has `type`, UUID-v4 `record_id`, UTC ISO-8601 `recorded_at`,
and the six sensor fields plus `device_uptime_ms`. Temperature must be
finite and within −50…100 °C, RH within 0…100, and illuminance nonnegative.
Invalid sensor values are null, never fabricated zeros. PIR is true if
HIGH was observed during a valid sampling interval, false otherwise,
and null during warm-up or an incompletely observed interval.

`received_at` is assigned by the server. Neither spatial differences nor
alternative record types are sent. Calculate ΔT = T2−T1 in °C and
ΔRH = RH2−RH1 in percentage points from the original values.

Acknowledgment requires HTTP 200 and all of:
`success == true`, `device_id == "ENV_GROUP_C_01"`, `accepted == 1`.
`accepted` counts validated request records, including duplicate IDs;
upsert with ignoreDuplicates makes retrying a saved record idempotent.

400/401/403/405/413/422 are quarantined for diagnosis. Transport failures,
unexpected responses and server failures remain queued for retry. UUID,
timestamp and sensor values do not change during retry. No data is deleted
because of a failed request.

Read-only dashboard endpoint:
`https://izbtxfmkfmkfledrqtpu.supabase.co/functions/v1/read-group-data`.
It uses the separate dashboard access code and must allow the Pages origin.
The device token and database service-role key must never be shipped to
the public dashboard.
