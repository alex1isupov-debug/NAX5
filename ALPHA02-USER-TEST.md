# NAX5 Alpha 0.2 user test

Use a local backend at `http://127.0.0.1:8000` or production `https://cloudgta6.com`. For local development:

```
set NAX5_API_BASE_URL=http://127.0.0.1:8000
```

Session tokens stay in RAM only. Closing NAX5 requires a new login.

PASS is recorded only for checks that actually ran.

## AUTOMATED

Recorded 2026-09-09 against branch `nax5-alpha-0.2` on this Windows PC.

- `nax5-auth-unit` compiled and executed: **PASS** (39 assertions, including Remote Play gated on `AUTHENTICATED`).
- Django `apps.accounts` tests: **PASS** (17).
- Django `apps.waitlist apps.consoles apps.sessions` tests: **PASS** (18).
- Real HTTP against local PostgreSQL backend at `http://127.0.0.1:8000`:
  - `GET /health/live` → 200 live: **PASS**
  - `GET /health/ready` → 200 ready: **PASS**
  - valid headless login → 200 + session token: **PASS**
  - invalid password → 400, not authenticated: **PASS**
  - valid `X-Session-Token` → `/api/v1/auth/me/` profile: **PASS**
  - invalid token → 401: **PASS**
  - logout (`DELETE /_allauth/app/v1/auth/session`) → 401, then `/me/` 401: **PASS** (this is the observed django-allauth 65.19.2 behaviour; the client still clears local state first)
  - unverified email login → 401, `verify_email` pending, not authenticated: **PASS**
- Clean Windows x64 Release build of `chiaki.exe`: **PASS**
- Portable package created with Qt DLLs, QML plugins, `platforms/qwindows.dll`, licenses, `UPSTREAM.md`, `THIRD-PARTY-NOTICES.md`, `ALPHA01-USER-TEST.md`, and `ALPHA02-USER-TEST.md`: **PASS**
- Portable `chiaki.exe` started from the packaged directory and stayed alive (~8s, ~450MB RSS, no stderr): **PASS** as process smoke only. Login UI content was **not** visually confirmed by this agent.

## MANUAL

Not confirmed by the user yet. Do these next, one step at a time:

1. Startup: LoginView only; no PS5 connection before login.
2. Wrong password.
3. Valid verified ACTIVE login; account bar email/city/accessStatus/emailVerified.
4. Logout → LoginView.
5. Login again.
6. Full restart → LoginView again (RAM session).
7. Backend offline login error.
8. Unverified account login message.
9. AutoConnect must not start before login.
10. Remote Play on the physical PS5 (video, audio, controller, fullscreen, disconnect, reconnect).
11. WAN public IPv4 session.
12. Alpha 0.1 vs 0.2 A/B gameplay.

## NOT TESTED

- Fresh Alpha 0.1 rebuild in this session (the existing 2026-09-07 Alpha 0.1 Release at `E:\fork\TASK2-WORK\build-alpha01` used the same MINGW64 toolchain; it was not rebuilt today).
- Visual confirmation of LoginView / MainView (requires the user).
- Physical PS5 / WAN / A/B gameplay.

## Local test accounts

Created only in the local PostgreSQL database. Do not commit passwords. After testing, delete them:

```
cd C:\astro\nax5-platform\apps\backend
.\.venv\Scripts\python.exe manage.py shell
```

```python
from django.contrib.auth import get_user_model
get_user_model().objects.filter(email__startswith="nax5.alpha02.").delete()
```

Then repeat the Alpha 0.1 Remote Play A/B checklist in [ALPHA01-USER-TEST.md](ALPHA01-USER-TEST.md) on the same PS5 and network.
