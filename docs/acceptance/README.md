# NAX5 acceptance tests

Current user acceptance checklist: [ALPHA05-USER-TEST.md](ALPHA05-USER-TEST.md).

Permanent automated/manual register: [../../TEST-MATRIX.md](../../TEST-MATRIX.md).

## History

| Release | Doc | Scope |
| --- | --- | --- |
| Alpha 0.1 | [history/ALPHA01-USER-TEST.md](history/ALPHA01-USER-TEST.md) | Branding, settings isolation, manual Chiaki registration |
| Alpha 0.2 | [history/ALPHA02-USER-TEST.md](history/ALPHA02-USER-TEST.md) | Native login against backend |
| Alpha 0.3 | [history/ALPHA03-USER-TEST.md](history/ALPHA03-USER-TEST.md) | Reserve console, no stream |
| Alpha 0.4 | [history/ALPHA04-USER-TEST.md](history/ALPHA04-USER-TEST.md) | Auto Remote Play, operator provisioning |
| **Alpha 0.5** | [ALPHA05-USER-TEST.md](ALPHA05-USER-TEST.md) | Production user pack, logs, client-reports, session hardening |

Local backend helpers stay in `scripts/nax5/start-nax5-local.cmd` and `scripts/nax5/start-nax5-operator-local.cmd`. Production user builds must **not** ship those scripts.
