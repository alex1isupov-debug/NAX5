#!/usr/bin/env bash
set -euo pipefail
export PATH="/mingw64/bin:/usr/bin:/bin:${PATH}"
export MSYSTEM=MINGW64
export LDD_TIMEOUT=10
ROOT=/c/astro/NAX5
BUILD="${NAX5_BUILD_DIR:-$ROOT/build-alpha04-acceptance}"
PORTABLE_ROOT="$BUILD/portable"
PORTABLE="$PORTABLE_ROOT/NAX5"
ZIP="$BUILD/NAX5-Alpha-0.4-Windows-x64.zip"
EXE="$BUILD/gui/chiaki.exe"
if [[ ! -f "$EXE" ]]; then
  echo "Missing $EXE — build Release chiaki.exe first."
  exit 1
fi
rm -rf "$PORTABLE_ROOT" "$ZIP"
mkdir -p "$PORTABLE"
cd "$ROOT"
bash scripts/deploy-windows-msys2.sh \
  "$PORTABLE" \
  "$EXE" \
  "$BUILD/third-party/cpp-steam-tools" \
  /mingw64 \
  gui/src/qml
cp -f scripts/nax5/start-nax5-local.cmd "$PORTABLE/start-nax5-local.cmd"
cp -f scripts/nax5/start-nax5-operator-local.cmd "$PORTABLE/start-nax5-operator-local.cmd"
cp -f ALPHA04-USER-TEST.md "$PORTABLE/ALPHA04-USER-TEST.md"
test -f "$PORTABLE/chiaki.exe"
test -f "$PORTABLE/COPYING"
test -d "$PORTABLE/LICENSES"
test -f "$PORTABLE/start-nax5-local.cmd"
test -f "$PORTABLE/start-nax5-operator-local.cmd"
test -f "$PORTABLE/ALPHA04-USER-TEST.md"
cd "$PORTABLE_ROOT"
zip -r -9 "$ZIP" NAX5
unzip -tq "$ZIP"
echo "PORTABLE=$PORTABLE"
echo "ZIP=$ZIP"
