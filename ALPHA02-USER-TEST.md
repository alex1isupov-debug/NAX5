# NAX5 Alpha 0.2 login test

Use a local backend at `http://127.0.0.1:8000` or production `https://cloudgta6.com`. For local development:

```
set NAX5_API_BASE_URL=http://127.0.0.1:8000
```

1. Create a website user with a verified email and `Profile.access_status=ACTIVE`.
2. Launch NAX5 Alpha 0.2. The login screen should appear.
3. Enter an incorrect password. Expect a clear Russian error and no authenticated UI.
4. Enter the correct password. Expect authenticated state with email, city, and access status.
5. Confirm the existing Remote Play host list remains available.
6. Click Выйти. Expect the login screen.
7. Log in again. Expect success.
8. Close NAX5 completely and reopen. Expect the login screen again (session token is RAM-only).

Then repeat the Alpha 0.1 Remote Play A/B checklist in [ALPHA01-USER-TEST.md](ALPHA01-USER-TEST.md) on the same PS5 and network.
