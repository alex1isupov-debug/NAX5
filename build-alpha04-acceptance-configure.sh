#!/usr/bin/env bash
set -euo pipefail
export PATH="/mingw64/bin:/usr/bin:/bin:${PATH}"
export MSYSTEM=MINGW64
export PKG_CONFIG_PATH="/mingw64/lib/pkgconfig"
source /e/fork/TASK2-WORK/python-venv/bin/activate
cd /c/astro/NAX5
BUILD="${NAX5_BUILD_DIR:-/c/astro/NAX5/build-alpha04-acceptance}"
echo "python=$(command -v python) $(python --version)"
echo "cmake=$(command -v cmake)"
echo "gcc=$(command -v gcc)"
echo "gxx=$(command -v c++)"
echo "ninja=$(command -v ninja)"
echo "qt6=$(ls /mingw64/lib/cmake/Qt6/Qt6Config.cmake)"
echo "build_dir=$BUILD"
cmake -S /c/astro/NAX5 -B "$BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCHIAKI_ENABLE_CLI=OFF \
  -DCHIAKI_ENABLE_TESTS=ON
