# Third-party notices for NAX5 Alpha 0.1

NAX5 Alpha 0.1 is a modified distribution of chiaki-ng 1.10.0. The distribution includes the project `COPYING` file and the source-tree `LICENSES` directory without rewriting their texts.

The Windows portable build also contains components used by the upstream build, including Qt 6, FFmpeg, libplacebo, SDL/sdl2-compat, OpenSSL, curl, protobuf/nanopb, Opus, SpeexDSP, Vulkan loader and related MSYS2 runtime libraries. Their inclusion here is a factual component inventory, not a replacement for their respective license texts or a claim that every component uses the same license.

Verified source-license locations include:

- `COPYING` and `LICENSES/` in this source tree;
- `third-party/curl/COPYING` and `third-party/curl/LICENSES/`;
- `third-party/cpp-steam-tools/LICENSE`;
- `third-party/nanopb/LICENSE.txt`;
- `third-party/jerasure/COPYING` and `third-party/jerasure/License.txt`;
- `third-party/gf-complete/COPYING` and `third-party/gf-complete/License.txt`;
- the license directories shipped with the reused MSYS2 packages;
- the license files shipped with the reused FFmpeg, libplacebo, SDL3, and sdl2-compat inputs, where present.

This Alpha package is prepared for limited external testing. A binary-to-license inventory and review of corresponding-source delivery requirements remains a release-compliance gap that must be closed before a public production release. No license obligation should be inferred from this summary alone; consult the unmodified license texts distributed with the relevant component.
