#!/usr/bin/env bash
set -euo pipefail
export PATH="/mingw64/bin:/usr/bin:/bin:${PATH}"
export MSYSTEM=MINGW64
source /e/fork/TASK2-WORK/python-venv/bin/activate
BUILD="${NAX5_BUILD_DIR:-/c/astro/NAX5/build-alpha04-acceptance}"
echo "building chiaki..."
echo "build_dir=$BUILD"
cmake --build "$BUILD" --config Release --target chiaki
echo "chiaki exit=$?"
ls -l "$BUILD/gui/chiaki.exe"
