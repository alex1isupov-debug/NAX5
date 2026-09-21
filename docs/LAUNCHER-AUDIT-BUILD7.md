# Launcher audit: Build 7 RC1

Baseline: `streetpea/chiaki-ng` v1.10.0. Audit target: the NAX5 product path,
diagnostic delivery path, Windows package and the locally available backend
contract. This is a release-candidate audit, not evidence from a completed live
player rollout.

## Findings closed in this candidate

| Severity | Finding | Resolution / evidence |
| --- | --- | --- |
| P0 | Full quit ZIP could exceed the backend 2 MiB limit and receive HTTP 413 | Session logs are journaled as independent ZIP parts capped below 1.5 MiB; backend-contract test accepts every synthetic part and rejects an oversized control archive. |
| P0 | A quit report was marked sent before HTTP success | The exact durable part is deleted only after HTTP 200/201/204. Failure retains the same bytes and schedules retry. |
| P0 | Shutdown could destroy an in-flight report | Capture is persisted before upload; shutdown finalizes the journal and waits for the report lane within a bounded grace period. |
| P1 | One process-wide quit flag suppressed reports from later Play sessions | Reports now have a distinct report/session identity and final manifest per Play lifecycle. |
| P1 | Logs from different accounts could be attributed incorrectly | Every journal/part stores the authenticated numeric owner ID; upload filters the shared queue by that owner. Legacy unattributed pending ZIPs are not reassigned. |
| P1 | Final `stream_connected` was lost after assignment cleanup | A report-scoped connection flag survives terminal API/session cleanup and resets before the next Play. |
| P1 | API immediate-failure signals could arrive before the request ID was stored | Completion delivery is queued, preserving request correlation. |
| P1 | Diagnostics changed renderer-owned state to produce a cumulative drop counter | Removed. Diagnostics read the existing one-second drop window and label its semantics explicitly. |
| P2 | Network hint could select an arbitrary active adapter | IPv4 default routes are ranked by metric; output is explicitly only a client-side hint and contains no IP/MAC. |
| P2 | Event batching failure could erase the only copy of an event | Events are also written to the complete process log before network batching/retry. |

## Static and automated coverage

- Native auth, session, connection and telemetry unit tests pass in MSYS2
  MINGW64.
- The telemetry regression reconstructs a source larger than 4 MiB byte for
  byte, verifies session boundaries, account isolation, retry immutability,
  exact acknowledgement, crash recovery, redaction and per-part limits.
- The safe offline reassembler has five tests and reports incomplete sequences
  with exit code 2.
- The Django multipart/storage contract accepts the generated C++ archives and
  preserves their bytes. This test mocks authentication/session DB and is not a
  live PostgreSQL/VPS test.
- Product-QML and no-host-persistence verification scripts pass.
- The full Windows client and Inno package compile; static imports contain no
  MSYS runtime dependency.
- No decoder, renderer, Remote Play protocol, audio, input, pacing, codec,
  crypto or streaming-socket implementation is changed by this diagnostic fix.

## Open release gates

| Priority | Gate | Required evidence |
| --- | --- | --- |
| P0 | Live delivery | Install this exact RC, run a real PS5 session, quit, download all report parts from the VPS and reconstruct a `COMPLETE` report. |
| P0 | Long/offline delivery | Repeat with a 60+ minute session and with network loss during quit; reconnect and verify every retained part uploads exactly once. |
| P1 | Account isolation | Queue a report offline under account A, sign in as account B, and prove A's part is not uploaded as B. Return to A and complete it. |
| P1 | Clean Windows launch | Run the standard clean-PATH install/launch smoke test. It was intentionally skipped for this RC because the launcher must not be opened automatically in the current environment. |
| P1 | Backend concurrency | Run `apps.client_telemetry` against real PostgreSQL and confirm the deadlock fix is deployed on the VPS; local mocked storage is insufficient. |
| P1 | Lag root-cause correlation | Correlate UTC client samples with router/PS5 Ethernet port errors, negotiated link, queue/load and a controlled wired comparison. Client logs distinguish receiver/network, decoder/render and application stalls but cannot identify a physical hop alone. |
| P2 | DLU error family | Reproduce the reported DLU-52-like UI errors from a clean installed profile and capture their exact text, state transition and API response. This RC was not launched, so no claim is made about those historical on-screen errors. |

Do not publish the RC as fully verified until the P0 gates pass. The P1 gates
may be accepted only as an explicit release risk with an owner and follow-up.
