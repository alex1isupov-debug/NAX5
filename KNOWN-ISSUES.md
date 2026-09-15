# KNOWN ISSUES

Findings from the Task 4.1 implementation and audit. RAM extraction of connection material is Task 5, not a Task 4 defect.

| ID | Severity | Component | Description | Reproduction | Impact | Root cause | Fix now / defer | Target |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| KI-001 | P1 | Task 3 manual | User A release → User B reserve is not user-confirmed | Two local accounts, one READY console | Capacity gate incomplete | Manual step not executed | Confirm before Alpha 0.5 WAN test | Alpha 0.5 |
| KI-011 | P1 | Product gate | Client Play requires `ACTIVE`; backend reserve accepts `INVITED` | INVITED user logs in | Play button disabled | Intentional client gate | Align site/admin or accept ACTIVE-only | Alpha 0.5 |
| KI-002 | TASK5 SECURITY RISK | NAX5 RAM | `registKey` / morning exist in process memory for the stream lifetime | Product Play after connection | User with local debugger can dump keys | Required for libchiaki session | Defer. Do not obfuscate | Task 5 |
| KI-003 | P2 | Vanilla parity | Public WAN IP uses remote 720p profile | `isLocalAddress(host)` false | Lower WAN resolution vs LAN | Upstream `StreamSessionConnectInfo` | Do not change in Task 4 | later quality task |
| KI-004 | P2 | Packaging | Clean Release portable and vanilla control build were not produced in this session | No `chiaki.exe` in the worktree | Manual operator test needs a package | MSYS2 build not run here | Build before physical PIN | Task 4 package |
| KI-005 | P3 | QML smoke | Headless Qt offscreen launcher smoke is not runnable without a built GUI binary | `QT_QPA_PLATFORM=offscreen` | Missing runtime QML crash net | No current GUI binary | Source-level QML assertions in place | Task 4 package |
| KI-006 | P3 | Operator UX | Operator provision uses the first operator `RegisteredHost` if display index 0 is empty | Operator panel Provision | Wrong host if several are registered | Smallest adapter | Operator should keep one lab host | later |
| KI-007 | P3 | Auth | Operator HTTP uses the same `csrf_exempt` native token pattern as Task 3 | Browser cookie session against operator URL | Not a browser app API | Alpha native client | Documented; staff still required | Task 5 if browser admin is added |
| KI-008 | P2 | Lifecycle | Client does not heartbeat ACTIVE sessions | Long play past 7200s | Server expires CONNECTING/ACTIVE after `SESSION_HARD_TIMEOUT_SECONDS` | Heartbeat is Task 6 | Temporary hard TTL only | Task 6 |
| KI-009 | P3 | Git | Nested `test/munit` worktree can make `git status` fail in this checkout | `git status` | Fork diff budget harder to compute | Nested baseline worktree | Use pathspec or a clean clone for budget | maintenance |
| KI-010 | P3 | Settings | Product Config tab remains (About / non-secret profile UI); credential export/import is hidden | Open Settings in Product | Extra upstream page still listed | Hide capability, not delete upstream | Acceptable for Alpha | later |

Fixed during Task 4.1 (not open):

- `lifecycle.py` `elif` without `if` (would not import)
- Operator test payload `session_id=operator-test` rejected by product session match
- Product `goToSleep()` could still send shared PS5 to sleep
- `syncCurrent` could abort an in-flight connection request
- SessionQuit during user cancel could mark FAILED
- Invalid env API URL no longer falls back to production
