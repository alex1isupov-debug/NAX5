# NAX5 Alpha 0.1 user test

This is an unsigned portable test build based on chiaki-ng v1.10.0. Windows SmartScreen may show “Windows protected your PC”; that reputation warning is not the same as an antivirus detection. Stop and report the result if Microsoft Defender reports a Trojan, PUA, or other threat.

1. Keep the vanilla control and NAX5 in separate extracted folders.
2. Launch each build using its own `chiaki.exe`, one at a time.
3. In vanilla, change one harmless setting and note its value. Launch NAX5 and confirm it did not inherit that value.
4. In NAX5, change the same setting. Relaunch vanilla and confirm vanilla did not change.
5. In NAX5, register the PS5 using the ordinary Chiaki registration flow. Do not reuse or copy a settings file from vanilla.
6. Run comparable A/B sessions with the same console, client PC, network, resolution, codec, bitrate, renderer, and controller.

For both vanilla and NAX5, check:

- launch and Settings;
- manual registration and connection;
- video and audio;
- controller input;
- fullscreen;
- disconnect and reconnect;
- a 20–30 minute session;
- WAN/mobile-hotspot behavior where practical.

Record whether NAX5 has parity with vanilla. PASS means no stable, repeatable degradation. If vanilla is good and NAX5 is systematically worse, stop feature work and report the exact setting, renderer, network, timestamps, and reproduction steps.

NAX5 Alpha 0.1 has no backend, NAX5 account, subscription, automatic console assignment, installer, code signing, auto-update, or one-click Play.
