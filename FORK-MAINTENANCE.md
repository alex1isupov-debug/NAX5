# NAX5 fork maintenance

Upstream: [streetpea/chiaki-ng](https://github.com/streetpea/chiaki-ng)

Baseline: chiaki-ng v1.10.0

Baseline SHA: `0c4a45df0cae2af2ba2daef84e881850b07038a3`

NAX5 version: Alpha 0.2

`nax5-baseline` is an exact snapshot of the upstream baseline. Do not commit product patches to that branch.

Product patches:

- P01 Application identity
- P02 Settings isolation
- P03 Windows metadata/package
- P04 NAX5 native backend authentication

Streaming core modifications: NONE

Protected areas (do not change unless a dedicated regression ADR requires it):

- `lib/src/session.c`
- `lib/include/chiaki/session.h`
- `gui/src/streamsession.cpp`
- Remote Play protocol, decoder, renderer, audio, controller, discovery, and PSN registration

NAX5-specific authentication lives under `gui/include/nax5/` and `gui/src/nax5/`. Session tokens stay in RAM for Alpha 0.2; passwords are not stored.
