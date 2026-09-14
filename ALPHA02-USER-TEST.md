# NAX5 Alpha 0.2 user test

Use a local backend at `http://127.0.0.1:8000` or production `https://cloudgta6.com`. For local development:

```
set NAX5_API_BASE_URL=http://127.0.0.1:8000
```

Do not double-click `chiaki.exe` for local tests. Without that variable NAX5 talks to production and shows «Сервис временно недоступен.» Helper: `build-alpha02-acceptance/portable/NAX5/start-nax5-local.cmd`.

Session tokens stay in RAM only. Closing NAX5 requires a new login.

PASS is recorded only for checks that actually ran.

## AUTOMATED

Recorded 2026-09-09 against branch `nax5-alpha-0.2` (`a05e6bc3` plus later doc updates) on this Windows PC.

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
  - logout (`DELETE /_allauth/app/v1/auth/session`) → 401, then `/me/` 401: **PASS** (observed django-allauth 65.19.2 behaviour; client still clears local RAM state first)
  - unverified email login → 401, `verify_email` pending, not authenticated: **PASS**
- Clean Windows x64 Release build of `chiaki.exe`: **PASS**
- Portable package with Qt DLLs, QML plugins, `platforms/qwindows.dll`, licenses, `UPSTREAM.md`, `THIRD-PARTY-NOTICES.md`, `ALPHA01-USER-TEST.md`, `ALPHA02-USER-TEST.md`: **PASS**
- Portable process smoke: **PASS** (process stayed alive). Login UI later confirmed by the user.

## MANUAL

User-confirmed 2026-09-09 with local backend and portable Alpha 0.2.

- Startup LoginView, no MainView before login: **PASS**
- Wrong password → «Неверный email или пароль.»: **PASS** (only when launched with `NAX5_API_BASE_URL`; without it the UI showed «Сервис временно недоступен.»)
- Valid verified ACTIVE login; account bar email + `ACTIVE`: **PASS**
- City / emailVerified on the account bar: **NOT SHOWN** (API `/me/` has them; bar only shows email and accessStatus)
- Logout → LoginView: **PASS**
- Login again: **PASS**
- Full restart → LoginView (RAM session): **PASS**
- Backend offline → «Нет подключения к интернету.», no crash: **PASS**
- Unverified account → «Подтвердите email перед входом.»: **PASS**
- AutoConnect / stream before login: **PARTIAL** — on LoginView no stream started. PS5 was powered off during that wait. After login, `PS5-439` at `192.168.1.9` appeared as **discovered / unregistered**. Alpha 0.1 registration did not carry over (settings isolation).
- Console registration: **IN PROGRESS** (Register Console dialog: host filled, PSN Account-ID and PIN still needed)
- Connect / video / audio / controller / fullscreen / disconnect / reconnect: **NOT TESTED**
- WAN public IPv4: **NOT TESTED**
- Alpha 0.1 vs 0.2 A/B gameplay: **NOT TESTED**

## NOT TESTED

- Fresh Alpha 0.1 rebuild in this session (existing 2026-09-07 Release at `E:\fork\TASK2-WORK\build-alpha01` used the same MINGW64 toolchain).
- Physical Remote Play session.
- WAN and A/B.

## Resume later

1. Keep PostgreSQL `nax5-task1-postgres-1` running.
2. Start backend: `apps/backend/.venv/Scripts/python.exe manage.py runserver 127.0.0.1:8000`
3. Launch NAX5 via `start-nax5-local.cmd` (not bare `chiaki.exe`).
4. Log in with the local verified user, finish PS5 register (`PSN Login` + PS5 `Settings → System → Remote Play → Link Device`), then connect.

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
