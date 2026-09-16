#!/usr/bin/env bash
# Back-compat wrapper: MSYS 7z SFX is not used. Native Inno Setup only.
set -euo pipefail

echo "MSYS 7z SFX packaging is disabled. Using Inno Setup instead." >&2
OUT="${1:-/c/astro/nax5-client/artifacts/alpha-0.5}"
USER_DIR="$OUT/portable/NAX5"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
exec bash "$ROOT/scripts/release/package-inno-launcher.sh" "$USER_DIR" "$OUT"
