# NAX5 Alpha 0.4 operator / user test

Use a local backend at `http://127.0.0.1:8000`.

## Launch

Product (fresh QSettings namespace `NAX5/NAX5`):

```
C:\astro\NAX5\scripts\nax5\start-nax5-local.cmd
```

After a portable package is built, run `start-nax5-local.cmd` from that folder. Do not double-click `chiaki.exe`. Without `NAX5_API_BASE_URL` NAX5 talks to production (`https://cloudgta6.com`).

Operator (separate QSettings namespace `NAX5/NAX5-Operator`):

```
C:\astro\NAX5\scripts\nax5\start-nax5-operator-local.cmd
```

`NAX5_OPERATOR_MODE=1` is a runtime profile, not a security boundary. Staff HTTP authorization is enforced on the backend.

## TASK 3 GATES (must stay green)

1. User A → Login → Play → reserve — previously PASS.
2. User B → Login → Play → `NO_CAPACITY` — previously PASS.
3. User A → Освободить, then User B → Play → reserve `PS5-439` — **not user-confirmed**. Do not mark Task 3 manual PASS until this step is confirmed.

## OPERATOR ACCEPTANCE (this stop)

Do not simulate a physical PS5. PASS only after a real PIN registration.

1. Backend: create Console in MAINTENANCE/PROVISIONING with `public_host` set to the reachable PS5 address.
2. Launch Operator NAX5.
3. Register the console with the original chiaki-ng flow: PS5 → Settings → System → Remote Play → Link Device → PIN.
4. Enter the backend console code and press **Provision selected host**.
5. Press **Test provisioned**. Confirm a vanilla stream starts.
6. Only after a successful test, press **Activate READY**.

Do not activate READY if provision write or the test connection failed.

## USER ACCEPTANCE (after operator PASS)

Fresh Product profile. `registered_hosts = 0`. `manual_hosts = 0`. No PSN registration.

Login → Play.

Expected: automatic Remote Play using backend connection material. No Register, PIN, IP, or PSN Account ID.

Then separately: video, audio, controller, fullscreen, disconnect, reconnect, WAN, 30+ minute soak, two users, vanilla A/B.

## NOT IN SCOPE

- Heartbeat (Task 6)
- RAM extraction of `registKey` / morning (Task 5)
- Changing vanilla WAN remote video profile
- New Remote Play protocol / decoder / renderer / audio / controller
