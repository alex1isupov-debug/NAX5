# NAX5 fork maintenance

Upstream: [streetpea/chiaki-ng](https://github.com/streetpea/chiaki-ng)

Baseline: chiaki-ng v1.10.0

Baseline SHA: `0c4a45df0cae2af2ba2daef84e881850b07038a3`

NAX5 version: Alpha 0.4

`nax5-baseline` is an exact snapshot of the upstream baseline. Do not commit product patches to that branch.

Product patches:

- P01 Application identity
- P02 Settings isolation
- P03 Windows metadata/package
- P04 NAX5 native backend authentication
- P05 Backend console assignment
- P06 Automatic Remote Play orchestration (transient StreamSessionConnectInfo, operator profile)

Streaming core modifications: NONE

Protected areas (do not change unless a dedicated regression ADR requires it):

- `lib/src/session.c`
- `lib/include/chiaki/session.h`
- `gui/src/streamsession.cpp`
- `gui/src/settings.cpp`
- `gui/src/host.cpp`
- Remote Play protocol, decoder, renderer, audio, controller, discovery, and PSN registration

NAX5-specific authentication lives under `gui/include/nax5/` and `gui/src/nax5/`. Console assignment lives under `gui/include/nax5/session/` and `gui/src/nax5/session/`. Connection material parsing and the transient host adapter live under `gui/include/nax5/connection/` and `gui/src/nax5/connection/`. Session tokens and connection keys stay in RAM; Product mode does not write `RegisteredHost` / `ManualHost` for Play. Operator mode uses QSettings `NAX5/NAX5-Operator` and the original chiaki-ng registration flow.

`nax5-baseline` is an exact snapshot of the upstream baseline. Do not commit product patches to that branch.

Product patches:

- P01 Application identity
- P02 Settings isolation
- P03 Windows metadata/package
- P04 NAX5 native backend authentication
- P05 Backend console assignment

Streaming core modifications: NONE

Protected areas (do not change unless a dedicated regression ADR requires it):

- `lib/src/session.c`
- `lib/include/chiaki/session.h`
- `gui/src/streamsession.cpp`
- Remote Play protocol, decoder, renderer, audio, controller, discovery, and PSN registration

NAX5-specific authentication lives under `gui/include/nax5/` and `gui/src/nax5/`. Console assignment lives under `gui/include/nax5/session/` and `gui/src/nax5/session/`. Session tokens stay in RAM; passwords are not stored. Play in Alpha 0.3 only calls the reservation API and does not start Remote Play.
