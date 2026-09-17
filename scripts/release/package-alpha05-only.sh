#!/usr/bin/env bash
# Package an already-built chiaki.exe into NAX5-windows.zip (no rebuild).
set -euo pipefail

export PATH="/mingw64/bin:/usr/bin:/bin:${PATH}"
export MSYSTEM=MINGW64
export LDD_TIMEOUT=10

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${NAX5_BUILD_DIR:-$ROOT/build-alpha05}"
OUT="${NAX5_RELEASE_DIR:-$ROOT/artifacts/alpha-0.5}"
if [[ -z "${GIT:-}" ]]; then
  GIT="/c/Program Files/Git/cmd/git.exe"
fi
STAGE="$OUT/portable/_stage"
USER_DIR="$OUT/portable/NAX5"
PORTABLE_ZIP="$OUT/NAX5-windows-portable.zip"
LEGACY_PORTABLE_ZIP="$OUT/NAX5-Alpha-0.5-Windows-x64.zip"
EXE="$BUILD/gui/chiaki.exe"
RELEASE_TAG="alpha-0.5-build-5"

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
rm -f "$USER_DIR"/start-nax5-*.cmd
rm -f "$USER_DIR"/ALPHA0*.md
cp -f "$ROOT/docs/acceptance/ALPHA05-USER-TEST.md" "$USER_DIR/ALPHA05-USER-TEST.md"
cp -f "$ROOT/scripts/release/README-USER-alpha05.txt" "$USER_DIR/README-USER.txt"

SHA="$("$GIT" -C "$ROOT" rev-parse HEAD)"
BRANCH="$("$GIT" -C "$ROOT" rev-parse --abbrev-ref HEAD)"
DESCRIBE="$("$GIT" -C "$ROOT" describe --tags --always --dirty)"
BUILT="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
EXE_SIZE="$(wc -c < "$USER_DIR/chiaki.exe" | tr -d '[:space:]')"
{
  echo "NAX5 Alpha 0.5 portable Windows x64"
  echo "release: ${RELEASE_TAG}"
  echo "profile: product"
  echo "repo: nax5-client"
  echo "branch: ${BRANCH}"
  echo "commit: ${SHA}"
  echo "describe: ${DESCRIBE}"
  echo "built: ${BUILT}"
  echo "configuration: Release"
  echo "chiaki.exe bytes: ${EXE_SIZE}"
  echo "default API: https://cloudgta6.com"
  echo "launch: NAX5.exe (Inno Setup installer) -> chiaki.exe"
  echo "launcher: NAX5.exe"
  echo "installer: NAX5-windows-installer.exe"
} > "$USER_DIR/BUILD-INFO.txt"

if ! compgen -G "$USER_DIR/avutil-*.dll" > /dev/null; then
  echo "Portable bundle is missing avutil-*.dll" >&2
  exit 1
fi

cd "$OUT/portable"
rm -f "$PORTABLE_ZIP" "$LEGACY_PORTABLE_ZIP"
zip -r -9 "$PORTABLE_ZIP" NAX5
unzip -tq "$PORTABLE_ZIP"
cp -f "$PORTABLE_ZIP" "$LEGACY_PORTABLE_ZIP"

bash "$ROOT/scripts/release/package-inno-launcher.sh" "$USER_DIR" "$OUT"

echo "PORTABLE_ZIP=$PORTABLE_ZIP"
ls -l "$PORTABLE_ZIP"
ls "$USER_DIR"/avutil-*.dll "$USER_DIR"/avcodec-*.dll
