# Build 7 RC1: full session diagnostic delivery

Baseline: chiaki-ng 1.10.0. This change does not alter the Remote Play protocol,
decoder, renderer, audio, or frame pacing. Existing uncommitted product changes
are included in the candidate and must be reviewed together before promotion.

## Delivery protocol

- On assignment, persist a journal with the authenticated numeric user ID and
  original session ID. Bind the exact `StreamSessionConnectInfo.log_file` before
  creating the stream; never select another user's "latest five" files.
- Read source logs incrementally in a worker thread. Each stored ZIP contains
  BUILD-INFO, MANIFEST.json and log-part.txt. Source chunks are at most 256 KiB,
  aligned to complete lines; no tail truncation is used by the automatic path.
- Persist ZIP parts atomically before upload. Commit the source cursor before a
  part becomes eligible to send. Preserve the same part when HTTP fails. Delete
  only that part after 200/201/204. Delivery is at-least-once: a lost ACK can cause
  a duplicate on the server. The analysis tool deduplicates by report/sequence.
- The final manifest lists source start/end offsets. Absence of that manifest,
  missing sequences, checksum failure, missing or truncated sources means
  INCOMPLETE, never "no errors" or "the network was fine".
- Capture during gameplay, upload only outside an active stream. One HTTP
  request at a time; exponential retry up to five minutes. Permanent-looking
  errors including 401/413 retain data and retry slowly. Login under a different
  account cannot upload another account's parts.
- Normal exit attempts transport stop, then grants report delivery up to ten
  seconds. Unsent parts remain under `log/report-queue-v2`. Unfinished journals
  are recovered as crash reports on the same account's next login.
- Existing legacy pending ZIPs have no trustworthy account/session identity:
  they are left untouched, not automatically attributed to a new login.

## Evidence for lag investigations

`nax5.metrics` JSON lines every two seconds include UTC and monotonic elapsed
time, sample gap, rolling packet-loss fraction, cumulative receiver lost frames,
presentation drops observed in the renderer's latest one-second window,
measured bitrate in kbps, render queue EMA,
pending frame age in ms and renderer enum.

`avg_packet_loss` in BUILD-INFO remains a compatibility field: it is the last
rolling sample over approximately two seconds, NOT a session-wide average.
Its maximum is the maximum sampled rolling value, not an exact maximum over
every packet. Measured bitrate is not configured bitrate. Decoder in BUILD-INFO
is configured decoder; stream log records initialization/fallback evidence.
`dropped_frames` in BUILD-INFO is the last observed renderer one-second window,
not a session total. The automatic diagnostics read existing renderer counters;
they do not add renderer state or alter render timing.
Default-route adapter type/link speed is only a client-side hint. It does not
measure Internet throughput or identify the route to the console when a VPN,
multiple routes or IPv6 is involved.

Interpret time-aligned evidence, not just counts: receiver loss/FEC errors,
decoder buffer warnings, render queue age/drops and application sampling gaps
are different observations. None identifies a bad PS5 cable, congested router,
ISP hop or client Wi-Fi on its own. Those need PS5-side port counters and/or a
controlled end-to-end network comparison. The VPS is not a video relay.

## Limits and privacy

Full means all available diagnostic lines for the recorded session after secret
redaction, not a network packet capture or memory dump. Product stream logging
forces sanitization. Upload redacts common keys/tokens/headers again. No login
credential or console connection material is added to the journal.

The spool has a 512 MiB safety ceiling. It never evicts unacknowledged parts to
make room; capture pauses and logs a warning. Disk failure, power loss before
flush, manual deletion, upstream rotation of not-yet-captured sources, or no
subsequent login can prevent complete delivery. A pathological individual line
over 256 KiB stops capture rather than silently dropping data. These conditions
must not be presented as a guarantee of full delivery. Existing source rotation
still keeps five prior files. Monitor backlog in beta before raising limits.

## Reconstruction

Download one user's ZIP files from the VPS into a local directory, then:

```text
python scripts/diagnostics/reassemble-reports.py INPUT_DIR --out NEW_OUTPUT_DIR
```

The output contains per-report full sanitized logs, BUILD-INFO.txt,
metrics.jsonl and summary.json. Exit 2 means an incomplete report. Legacy ZIPs
are skipped. The tool never extracts archive paths or connects to a database.

## Verification and release gate

- `nax5-telemetry-unit`: >4 MiB source reconstructed byte-for-byte, cap per part,
  owner isolation, immutable retry, precise ACK deletion, crash recovery, secret
  redaction. Tests use synthetic logs, no user credentials.
- `scripts/diagnostics/test-reassemble.py`: complete, missing part, missing final,
  overlap/gap, duplicate ACK retry, unsafe path.
- `test-backend-contract.py BACKEND FIXTURES`: real Django multipart parser and
  file storage, with authentication/session database calls mocked. This is not
  a PostgreSQL integration test or proof of live VPS delivery.
- Native unit EXEs run only in MSYS2 MINGW64 with the proper DLL PATH.
- Build with the standard release script from an isolated committed snapshot;
  the main working tree retains the user's uncommitted changes.
- `NAX5_SKIP_LAUNCH_SMOKE=1` skips the interactive/silent-install smoke test and
  marks it NOT TESTED. It must not be silently treated as passed.

Before public promotion: clean-Windows installation/DLL check; controlled real
PS5 session, Disconnect -> Play again, long session, abrupt termination,
offline/reconnect, logout/login as another account; verify reports reach the
VPS with the correct session IDs and reconstruct COMPLETE. Server deadlock
fixes for client-events are a separate backend deployment, not delivered by a
Windows installer. Do not claim they are live based on local code alone.
