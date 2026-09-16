#!/usr/bin/env bash
# Build and package NAX5 Alpha 0.5 production user installer (+ portable ZIP for release).
# Run from MSYS2 MINGW64.
set -euo pipefail

export PATH="/mingw64/bin:/usr/bin:/bin:${PATH}"
export MSYSTEM=MINGW64
export PKG_CONFIG_PATH="/mingw64/lib/pkgconfig"
export LDD_TIMEOUT=10

if ! /mingw64/bin/python3 -c 'import google.protobuf' >/dev/null 2>&1; then
  echo "Missing mingw-w64-x86_64-python-protobuf. Install with:" >&2
  echo "  pacman -S mingw-w64-x86_64-python-protobuf" >&2
  exit 1
fi

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${NAX5_BUILD_DIR:-$ROOT/build-alpha05}"
OUT="${NAX5_RELEASE_DIR:-$ROOT/artifacts/alpha-0.5}"
GIT="${GIT:-/c/Program Files/Git/cmd/git.exe}"
STAGE="$OUT/portable/_stage"
USER_DIR="$OUT/portable/NAX5-user"
USER_ZIP="$OUT/NAX5-Alpha-0.5-Windows-x64.zip"
INSTALLER="$OUT/NAX5-windows-installer.exe"
EXE="$BUILD/gui/chiaki.exe"
RELEASE_TAG="alpha-0.5-build-1"

cd "$ROOT"

if [[ ! -f "$ROOT/gui/nax5.ico" ]]; then
  /mingw64/bin/python3 "$ROOT/scripts/branding/generate-nax5-ico.py"
fi

SHA="$("$GIT" -C "$ROOT" rev-parse HEAD)"
BRANCH="$("$GIT" -C "$ROOT" rev-parse --abbrev-ref HEAD)"
DESCRIBE="$("$GIT" -C "$ROOT" describe --tags --always --dirty)"
mapfile -t DIRTY < <("$GIT" -C "$ROOT" status --porcelain")
DIRTY_COUNT="${#DIRTY[@]}"

echo "HEAD=$SHA"
echo "DESCRIBE=$DESCRIBE"
echo "BRANCH=$BRANCH"
echo "DIRTY_COUNT=$DIRTY_COUNT"

if [[ "$DIRTY_COUNT" -ne 0 ]]; then
  echo "Refusing to build a release tag with a dirty worktree:" >&2
  printf '  %s\n' "${DIRTY[@]}" >&2
  exit 1
fi

cmake -S "$ROOT" -B "$BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCHIAKI_ENABLE_CLI=OFF \
  -DCHIAKI_ENABLE_TESTS=ON

echo "building chiaki..."
cmake --build "$BUILD" --config Release --target chiaki
echo "building unit tests..."
cmake --build "$BUILD" --config Release --target nax5-auth-unit nax5-session-unit nax5-connection-unit

"$BUILD/gui/nax5-auth-unit.exe"
"$BUILD/gui/nax5-session-unit.exe"
"$BUILD/gui/nax5-connection-unit.exe"
echo "unit tests passed"

test -f "$EXE"

EXE_WIN="$(cygpath -w "$EXE")"
DESC="$(powershell.exe -NoProfile -Command "[System.Diagnostics.FileVersionInfo]::GetVersionInfo('$EXE_WIN').FileDescription" | tr -d '\r')"
ORIG="$(powershell.exe -NoProfile -Command "[System.Diagnostics.FileVersionInfo]::GetVersionInfo('$EXE_WIN').OriginalFilename" | tr -d '\r')"
if [[ "$DESC" != "NAX5 Remote Play Client" ]]; then
  echo "Unexpected FileDescription: [$DESC]" >&2
  exit 1
fi
if [[ "$ORIG" != "chiaki.exe" ]]; then
  echo "Unexpected OriginalFilename: [$ORIG]" >&2
  exit 1
fi

rm -rf "$STAGE" "$USER_DIR" "$USER_ZIP"
mkdir -p "$STAGE" "$OUT/portable"

bash "$ROOT/scripts/deploy-windows-msys2.sh" \
  "$STAGE" \
  "$EXE" \
  "$BUILD/third-party/cpp-steam-tools" \
  /mingw64 \
  gui/src/qml

printf '%s\n' '[Paths]' 'Prefix = .' > "$STAGE/qt.conf"

BUILT="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
EXE_SIZE="$(wc -c < "$STAGE/chiaki.exe" | tr -d "[:space:]")"

cp -a "$STAGE" "$USER_DIR"
rm -f "$USER_DIR"/start-nax5-*.cmd
rm -f "$USER_DIR"/ALPHA0*.md

cp -f "$ROOT/docs/acceptance/ALPHA05-USER-TEST.md" "$USER_DIR/ALPHA05-USER-TEST.md"
cp -f "$ROOT/scripts/release/README-USER-alpha05.txt" "$USER_DIR/README-USER.txt"

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
  echo "launch: chiaki.exe"
  echo "installer: ${INSTALLER##*/}"
  echo "unit tests: nax5-auth-unit, nax5-session-unit, nax5-connection-unit passed"
  echo "features: login, reserve, auto Remote Play, process log, client-reports, session hardening"
} > "$USER_DIR/BUILD-INFO.txt"

if find "$USER_DIR" -iname '*.cmd' | grep -q .; then
  echo "User pack must not contain .cmd launchers" >&2
  exit 1
fi

cd "$OUT/portable"
rm -f "$USER_ZIP"
zip -r -9 "$USER_ZIP" NAX5-user
unzip -tq "$USER_ZIP"

SHA256="$(sha256sum "$USER_ZIP" | awk '{print $1}')"
echo "USER_ZIP=$USER_ZIP"
echo "SHA256=$SHA256"
ls -l "$USER_ZIP"

ISCC=""
if [[ -f '/c/Program Files (x86)/Inno Setup 6/ISCC.exe' ]]; then
  ISCC='/c/Program Files (x86)/Inno Setup 6/ISCC.exe'
elif [[ -f '/c/Program Files/Inno Setup 6/ISCC.exe' ]]; then
  ISCC='/c/Program Files/Inno Setup 6/ISCC.exe'
elif [[ -f "/c/Users/${USERNAME}/AppData/Local/Programs/Inno Setup 6/ISCC.exe" ]]; then
  ISCC="/c/Users/${USERNAME}/AppData/Local/Programs/Inno Setup 6/ISCC.exe"
fi

if [[ -z "$ISCC" ]]; then
  echo "Inno Setup 6 (ISCC.exe) is required to build NAX5-windows-installer.exe" >&2
  echo "Install from https://jrsoftware.org/isinfo.php and re-run this script." >&2
  exit 1
fi

rm -f "$INSTALLER"
"$ISCC" \
  "/DMyAppPath=$(cygpath -w "$USER_DIR")" \
  "/DMyOutputDir=$(cygpath -w "$OUT")" \
  "/DMyOutputBase=NAX5-windows-installer" \
  "$(cygpath -w "$ROOT/scripts/nax5-windows-user.iss")"
test -f "$INSTALLER"
INSTALLER_SIZE="$(wc -c < "$INSTALLER" | tr -d "[:space:]")"
echo "INSTALLER=$INSTALLER"
echo "INSTALLER_BYTES=$INSTALLER_SIZE"
ls -l "$INSTALLER"

echo "DONE"
