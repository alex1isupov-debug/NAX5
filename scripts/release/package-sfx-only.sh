#!/usr/bin/env bash
set -euo pipefail
export PATH="/mingw64/bin:/usr/bin:/bin:${PATH}"
export MSYSTEM=MINGW64

OUT="${1:-/c/astro/nax5-client/artifacts/alpha-0.5}"
USER_DIR="$OUT/portable/NAX5"
LAUNCHER_EXE="$OUT/NAX5.exe"
LAUNCHER_ZIP="$OUT/NAX5-windows.zip"

test -d "$USER_DIR"
test -f "$USER_DIR/chiaki.exe"
test -f "$USER_DIR/avutil-59.dll" || compgen -G "$USER_DIR/avutil-*.dll" >/dev/null
test -f "$USER_DIR/avformat-61.dll" || compgen -G "$USER_DIR/avformat-*.dll" >/dev/null

SFX_MODULE=""
for candidate in \
  /usr/lib/p7zip/7zSD.sfx \
  /usr/lib/p7zip/7zS.sfx \
  /usr/lib/p7zip/7zCon.sfx \
  /mingw64/lib/p7zip/7zSD.sfx \
  "/c/Program Files/7-Zip/7zSD.sfx" \
  "/c/Program Files (x86)/7-Zip/7zSD.sfx"
do
  if [[ -f "$candidate" ]]; then
    SFX_MODULE="$candidate"
    break
  fi
done
test -n "$SFX_MODULE"

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
unzip -l "$LAUNCHER_ZIP"
