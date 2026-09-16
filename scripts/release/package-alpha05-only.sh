#!/usr/bin/env bash
# Package an already-built chiaki.exe into NAX5-windows.zip (no rebuild).
set -euo pipefail

export PATH="/mingw64/bin:/usr/bin:/bin:${PATH}"
export MSYSTEM=MINGW64
export LDD_TIMEOUT=10

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${NAX5_BUILD_DIR:-$ROOT/build-alpha05}"
OUT="${NAX5_RELEASE_DIR:-$ROOT/artifacts/alpha-0.5}"
STAGE="$OUT/portable/_stage"
USER_DIR="$OUT/portable/NAX5"
PORTABLE_ZIP="$OUT/NAX5-windows-portable.zip"
LEGACY_PORTABLE_ZIP="$OUT/NAX5-Alpha-0.5-Windows-x64.zip"
LAUNCHER_EXE="$OUT/NAX5.exe"
LAUNCHER_ZIP="$OUT/NAX5-windows.zip"
EXE="$BUILD/gui/chiaki.exe"

test -f "$EXE"

rm -rf "$STAGE" "$USER_DIR"
mkdir -p "$STAGE" "$OUT/portable"

bash "$ROOT/scripts/deploy-windows-msys2.sh" \
  "$STAGE" \
  "$EXE" \
  "$BUILD/third-party/cpp-steam-tools" \
  /mingw64 \
  gui/src/qml

printf '%s\n' '[Paths]' 'Prefix = .' > "$STAGE/qt.conf"
cp -a "$STAGE" "$USER_DIR"

if ! compgen -G "$USER_DIR/avutil-*.dll" > /dev/null; then
  echo "Portable bundle is missing avutil-*.dll" >&2
  exit 1
fi

cd "$OUT/portable"
rm -f "$PORTABLE_ZIP" "$LEGACY_PORTABLE_ZIP"
zip -r -9 "$PORTABLE_ZIP" NAX5

if ! command -v 7z >/dev/null; then
  echo "p7zip is required (pacman -S p7zip)" >&2
  exit 1
fi

SFX_MODULE=""
for candidate in \
  /usr/lib/p7zip/7zSD.sfx \
  /mingw64/lib/p7zip/7zSD.sfx \
  "/c/Program Files/7-Zip/7zSD.sfx" \
  "/c/Program Files (x86)/7-Zip/7zSD.sfx"
do
  if [[ -f "$candidate" ]]; then
    SFX_MODULE="$candidate"
    break
  fi
done

if [[ -z "$SFX_MODULE" ]]; then
  echo "7zSD.sfx not found" >&2
  exit 1
fi

SFX_CONFIG="$OUT/sfx-config.txt"
SFX_ARCHIVE="$OUT/nax5-portable.7z"
cat > "$SFX_CONFIG" << 'EOF'
;!@Install@!UTF-8!
Title="NAX5"
GUIMode="2"
OverwriteMode="2"
RunProgram="chiaki.exe"
;!@InstallEnd@!
EOF

rm -f "$SFX_ARCHIVE" "$LAUNCHER_EXE" "$LAUNCHER_ZIP"
(
  cd "$USER_DIR"
  7z a -mx=9 -t7z "$SFX_ARCHIVE" . >/dev/null
)
cat "$SFX_MODULE" "$SFX_CONFIG" "$SFX_ARCHIVE" > "$LAUNCHER_EXE"
rm -f "$SFX_CONFIG" "$SFX_ARCHIVE"

(
  cd "$OUT"
  zip -9 "$(basename "$LAUNCHER_ZIP")" "$(basename "$LAUNCHER_EXE")"
)

echo "LAUNCHER_ZIP=$LAUNCHER_ZIP"
ls -l "$LAUNCHER_ZIP" "$LAUNCHER_EXE"
ls "$USER_DIR"/avutil-*.dll "$USER_DIR"/avcodec-*.dll
