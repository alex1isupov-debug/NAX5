#!/usr/bin/env bash
set -euo pipefail
export PATH="/mingw64/bin:/usr/bin:/bin:${PATH}"
export MSYSTEM=MINGW64
source /e/fork/TASK2-WORK/python-venv/bin/activate
BUILD="${NAX5_BUILD_DIR:-/c/astro/NAX5/build-alpha04-acceptance}"
cmake --build "$BUILD" --config Release --target nax5-auth-unit nax5-session-unit nax5-connection-unit
echo "=== running nax5-auth-unit ==="
"$BUILD/gui/nax5-auth-unit.exe"
echo "=== running nax5-session-unit ==="
"$BUILD/gui/nax5-session-unit.exe"
echo "=== running nax5-connection-unit ==="
"$BUILD/gui/nax5-connection-unit.exe"
