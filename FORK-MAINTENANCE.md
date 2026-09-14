# NAX5 fork maintenance

Upstream: [streetpea/chiaki-ng](https://github.com/streetpea/chiaki-ng)

Baseline: chiaki-ng v1.10.0 (`nax5-baseline` is an exact snapshot). Do not commit product patches to that branch.

Product patches:

- P01 Application identity
- P02 Settings isolation
- P03 Windows metadata/package
- P04 NAX5 native backend authentication
- P05 Backend console assignment
- P06 Automatic Remote Play orchestration (transient `StreamSessionConnectInfo`, operator profile)

Streaming core modifications: none.

Protected areas (do not change unless a dedicated architectural decision requires it):

- `lib/src/session.c`
- `lib/include/chiaki/session.h`
- `gui/src/streamsession.cpp`
- `gui/src/settings.cpp`
- `gui/src/host.cpp`
- Remote Play protocol, decoder, renderer, audio, controller, discovery, and PSN registration

NAX5 C++ product code lives under `gui/include/nax5/` and `gui/src/nax5/`. Session tokens and connection keys stay in RAM. Product mode does not write `RegisteredHost` / `ManualHost` for Play. Operator mode uses QSettings `NAX5/NAX5-Operator` and the original chiaki-ng registration flow.

Developer guide: [NAX5.md](NAX5.md).
