# NAX5 Alpha 0.5 user test

Production user pack for Windows x64. Default API: `https://cloudgta6.com`.

Build with `scripts/release/build-alpha05-user-pack.sh`. Artifact: `artifacts/alpha-0.5/NAX5-Alpha-0.5-Windows-x64.zip`.

## Install

1. Download `NAX5-Alpha-0.5-Windows-x64.zip` from the GitHub release.
2. Extract the **entire** archive to a new folder.
3. Run `chiaki.exe` from that folder. Do not set `NAX5_API_BASE_URL` or `NAX5_OPERATOR_MODE`.
4. SmartScreen may warn on the unsigned build — expected. Stop if Defender reports a real threat.

## Account

Register on [cloudgta6.com/register/](https://www.cloudgta6.com/register/), confirm email, wait for **`ACTIVE`** access status (Play is gated on `ACTIVE`, not `INVITED`).

## Logs

| Location | Files |
| --- | --- |
| `%AppData%\Roaming\NAX5\NAX5\log\` | `nax5_*.log` (control plane), `chiaki_session_*.log` (stream) |
| Desktop ZIP | «Сохранить отчёт» — last 5 session logs + current process log |
| Server | `POST /api/v1/client-reports/` on apex — tail only, when logged in |

## AUTOMATED (required before tagging)

Run by `scripts/release/build-alpha05-user-pack.sh`:

- `nax5-auth-unit`, `nax5-session-unit`, `nax5-connection-unit` — **PASS required**
- Portable tree checks (`chiaki.exe`, Qt DLLs, `qt.conf`, no `.cmd`, no operator files)
- `FileDescription` = `NAX5 Remote Play Client`

## MANUAL — product (production)

Do not mark PASS without user confirmation on a clean Windows 11 PC.

1. Login with verified **ACTIVE** account — **NOT TESTED**
2. Play → reserve → automatic Remote Play (no Register / IP / PIN / PSN Account ID) — **NOT TESTED**
3. Video, audio, controller, fullscreen — **NOT TESTED**
4. Disconnect → Play again — **NOT TESTED**
5. 20–30 minute soak — **NOT TESTED**
6. Second user → `NO_CAPACITY` while first occupies console — **NOT TESTED**
7. WAN / mobile hotspot from another network — **NOT TESTED**
8. «Сохранить отчёт» → ZIP on Desktop + server report when authenticated — **NOT TESTED**

## MANUAL — operator (separate build)

Operator pack is **not** included in the Alpha 0.5 user ZIP. Use `scripts/nax5/start-nax5-operator-local.cmd` against a dev backend only.

Physical PS5 PIN registration and READY activation follow [history/ALPHA04-USER-TEST.md](history/ALPHA04-USER-TEST.md).

## NOT IN SCOPE

- Session heartbeat (Task 6)
- Code signing / SmartScreen reputation
- Auto-update
- `INVITED` Play gate (client requires `ACTIVE`)
