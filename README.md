# NAX5

NAX5 is a minimal Windows client for remote access to a physical PlayStation 5 over the Internet.

Developer guide: [NAX5.md](NAX5.md).

**Current release:** NAX5 Alpha 0.2 for Windows x64

**Remote Play engine:** chiaki-ng v1.10.0

[Download NAX5 Alpha 0.1](https://github.com/alex1isupov-debug/NAX5/releases/tag/alpha-0.1-build-1)

## Alpha 0.2

Alpha 0.2 adds a native NAX5 login screen. Use the same email and password created on [cloudgta6.com](https://cloudgta6.com/register/). After login, NAX5 shows the account email and access status, then the existing Remote Play UI.

Production talks to `https://cloudgta6.com`. For a local backend, set `NAX5_API_BASE_URL=http://127.0.0.1:8000` before launching. Session tokens stay in memory only; closing NAX5 requires a new login.

See [ALPHA02-USER-TEST.md](ALPHA02-USER-TEST.md) for the login checklist and [ALPHA01-USER-TEST.md](ALPHA01-USER-TEST.md) for Remote Play A/B testing.

## Alpha 0.1

This release answers one narrow question: can NAX5 add its own product identity and isolated settings while preserving the behavior of the proven chiaki-ng Remote Play engine?

Alpha 0.1 provides:

- NAX5 application identity and Windows product metadata;
- settings and registered-console storage isolated from official Chiaki;
- ordinary Chiaki PS5 registration and connection flows;
- a portable Windows x64 ZIP with no installer;
- the upstream video, audio, controller, networking, decoder, and renderer implementation unchanged.

The executable inside the portable folder is currently named `chiaki.exe`. This internal filename is intentional for Alpha 0.1.

## Install and test

1. Download `NAX5-Alpha-0.1-Windows-x64.zip` from the release page.
2. Extract the complete ZIP to a new folder.
3. Run `chiaki.exe` from that folder.
4. Register the PS5 through the standard Chiaki registration flow.
5. Connect and test video, audio, controller input, fullscreen, disconnect, and reconnect.

This build is unsigned. Windows SmartScreen may show a reputation warning. Do not disable Microsoft Defender or add exclusions; stop if Defender reports an actual threat.

See [ALPHA01-USER-TEST.md](ALPHA01-USER-TEST.md) for the A/B test procedure.

## Current limitations

Alpha 0.2 has no persistent login, automatic console assignment, Play button, installer, code signing, auto-update, or one-click Play. The upstream technical settings UI remains available for testing and diagnostics.

## Upstream and licensing

NAX5 Alpha 0.2 is a modified distribution of [streetpea/chiaki-ng](https://github.com/streetpea/chiaki-ng), based on tag `v1.10.0` at commit `0c4a45df0cae2af2ba2daef84e881850b07038a3`.

NAX5 modifications are intentionally limited to product identity, settings isolation, Windows metadata, documentation, portable packaging, and native backend authentication. Original licenses, SPDX headers, copyright notices, and upstream attribution are retained. See [UPSTREAM.md](UPSTREAM.md), [FORK-MAINTENANCE.md](FORK-MAINTENANCE.md), [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md), [COPYING](COPYING), and [LICENSES](LICENSES).

NAX5 is an independent project and is not affiliated with, endorsed by, sponsored by, or certified by Sony Interactive Entertainment Inc.
