# NAX5 Alpha 0.3 user test

Use a local backend at `http://127.0.0.1:8000`. For local development:

```
set NAX5_API_BASE_URL=http://127.0.0.1:8000
```

Do not double-click `chiaki.exe` for local tests. Without that variable NAX5 talks to production. Helper: `build-alpha03-acceptance/portable/NAX5/start-nax5-local.cmd`.

Session tokens stay in RAM only. Closing NAX5 requires a new login.

Alpha 0.3 Play reserves a console through the backend. It does **not** start Remote Play, inject a Host, or use PS5 credentials.

PASS is recorded only for checks that actually ran.

## AUTOMATED

Recorded 2026-09-09 against branch `task3-console-assignment` (`a05e6bc3` plus Task 3 commits) on this Windows PC.

- `nax5-auth-unit`: **PASS**
- `nax5-session-unit`: **PASS**
- Django `apps.accounts apps.waitlist apps.consoles apps.sessions`: **PASS** (53)
- Django session API concurrency (2 users and 20 users, one console): **PASS**
- Real PostgreSQL API test User A reserve / User B `NO_CAPACITY` / User A cancel / User B reserve `PS5-439`: **PASS**
- Clean Windows x64 Release `chiaki.exe`: **PASS**
- Portable package `build-alpha03-acceptance/portable/NAX5` and `NAX5-Alpha-0.3-Windows-x64.zip`: **PASS**

## MANUAL

Do not mark PASS without user confirmation.

First scenario:

1. User A → Login → Play
2. Expected: `Консоль выделена` and `PS5-439`
3. User B → Login → Play
4. Expected: `Все консоли сейчас заняты.`
5. User A → Освободить
6. User B → Play
7. Expected: reservation succeeds for `PS5-439`

## NOT IN SCOPE

- Automatic Remote Play connect
- Credential delivery (`regist_key`, morning, PIN, PSN)
- Session heartbeat (Task 6)
- CONNECTING / ACTIVE gameplay states (Task 4 / Task 6)

## Resume later

1. Keep PostgreSQL `nax5-task1-postgres-1` running.
2. Start backend: `apps/backend/.venv/Scripts/python.exe manage.py runserver 127.0.0.1:8000`
3. Launch NAX5 via `start-nax5-local.cmd`.
4. Log in with the local verified ACTIVE users created for this test.

## Local test accounts

Created only in the local PostgreSQL database. Do not commit passwords. The operator will receive the local User A / User B credentials with the manual acceptance note.

After testing, delete them from Django admin or:

```
cd C:\astro\nax5-platform\apps\backend
.\.venv\Scripts\python.exe manage.py shell
```
