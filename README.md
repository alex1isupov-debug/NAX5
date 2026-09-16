# NAX5

NAX5 is a Windows client for remote access to a physical PlayStation 5 over the Internet.

Developer guide: [NAX5.md](NAX5.md). Acceptance tests: [docs/acceptance/ALPHA05-USER-TEST.md](docs/acceptance/ALPHA05-USER-TEST.md).

**Current release:** NAX5 Alpha 0.5 for Windows x64 (branch `task4-auto-remote-play`)

**Remote Play engine:** chiaki-ng v1.10.0

[Download releases](https://github.com/alex1isupov-debug/NAX5/releases)

## Alpha 0.5

Alpha 0.5 is the first **production user** build intended for closed testers:

- native login with the same email/password as [cloudgta6.com](https://www.cloudgta6.com/register/);
- Play → backend reserve → automatic Remote Play (no manual PS5 registration);
- process logging and optional client-reports upload to the control plane;
- Windows installer `NAX5-windows-installer.exe` (Start menu shortcut to `chiaki.exe`).

Production API: `https://cloudgta6.com`. Play requires verified email and **`ACTIVE`** access status.

For local development only, set `NAX5_API_BASE_URL=http://127.0.0.1:8000` and use `scripts/nax5/start-nax5-local.cmd`. **Do not** set that variable in the production user pack.

## Install and test

1. Download `NAX5-windows-installer.exe` from GitHub Releases.
2. Run the installer and launch NAX5 from the Start menu.
3. Log in, press **Играть**, confirm video/audio/controller.

This build is unsigned. Windows SmartScreen may show a reputation warning. Do not disable Microsoft Defender; stop if Defender reports an actual threat.

See [docs/acceptance/ALPHA05-USER-TEST.md](docs/acceptance/ALPHA05-USER-TEST.md) for the full checklist.

Build the launcher: [docs/release/BUILD-LAUNCHER.md](docs/release/BUILD-LAUNCHER.md).

## Earlier alphas

Historical acceptance notes live under [docs/acceptance/history/](docs/acceptance/history/).

| Version | Focus |
| --- | --- |
| 0.1 | Branding + manual Chiaki registration only |
| 0.2 | Native login |
| 0.3 | Console reserve, no stream |
| 0.4 | Auto Remote Play + operator provisioning |

## Upstream and licensing

NAX5 is a modified distribution of [streetpea/chiaki-ng](https://github.com/streetpea/chiaki-ng), based on tag `v1.10.0` at commit `0c4a45df0cae2af2ba2daef84e881850b07038a3`.

See [UPSTREAM.md](UPSTREAM.md), [FORK-MAINTENANCE.md](FORK-MAINTENANCE.md), [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md), [COPYING](COPYING), and [LICENSES](LICENSES).

NAX5 is an independent project and is not affiliated with, endorsed by, sponsored by, or certified by Sony Interactive Entertainment Inc.
