#!/usr/bin/env bash
# Build and package NAX5 Alpha 0.5 production user ZIP (+ optional Inno installer).
# Run from MSYS2 MINGW64. Requires: cmake, ninja, gcc, zip, git, deploy-windows-msys2.sh deps.
set -euo pipefail

export PATH="/mingw64/bin:/usr/bin:/bin:${PATH}"
export MSYSTEM=MINGW64
export PKG_CONFIG_PATH="/mingw64/lib/pkgconfig"
export LDD_TIMEOUT=10

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="${NAX5_BUILD_DIR:-$ROOT/build-alpha05}"
OUT="${NAX5_RELEASE_DIR:-$ROOT/artifacts/alpha-0.5}"
GIT="${GIT:-/c/Program Files/Git/cmd/git.exe}"
STAGE="$OUT/portable/_stage"
USER_DIR="$OUT/portable/NAX5-user"
USER_ZIP="$OUT/NAX5-Alpha-0.5-Windows-x64.zip"
EXE="$BUILD/gui/chiaki.exe"
RELEASE_TAG="alpha-0.5-build-1"

cd "$ROOT"

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

DESC="$(powershell.exe -NoProfile -Command "[System.Diagnostics.FileVersionInfo]::GetVersionInfo('$(cygpath -w "$EXE")').FileDescription" | tr -d '\r')"
ORIG="$(powershell.exe -NoProfile -Command "[System.Diagnostics.FileVersionInfo]::GetVersionInfo('$(cygpath -w "$EXE")').OriginalFilename" | tr -d '\r')"
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

cat > "$STAGE/qt.conf" <<'EOF'
[Paths]
Prefix = .
EOF

BUILT="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
EXE_SIZE="$(wc -c < "$STAGE/chiaki.exe" | tr -d ' ')"

cp -a "$STAGE" "$USER_DIR"
rm -f "$USER_DIR"/start-nax5-*.cmd
rm -f "$USER_DIR"/ALPHA0*.md

cp -f "$ROOT/docs/acceptance/ALPHA05-USER-TEST.md" "$USER_DIR/ALPHA05-USER-TEST.md"

cat > "$USER_DIR/README-USER.txt" <<'EOF'
NAX5 Alpha 0.5 — portable Windows x64

1. Extract the full ZIP to a new folder. Do not run from inside the archive.
2. Launch chiaki.exe from that folder. Keep DLLs, qml, platforms, and qt.conf together.
3. API base is https://cloudgta6.com — do not set NAX5_API_BASE_URL.
4. Do not set NAX5_OPERATOR_MODE.
5. SmartScreen warnings are expected for this unsigned alpha build.
6. PS5 should be in rest mode, not fully powered off.
7. Play requires verified email and ACTIVE access status on cloudgta6.com.
8. Logs: %AppData%\Roaming\NAX5\NAX5\log\
9. «Сохранить отчёт» saves a ZIP to Desktop and uploads log tails when logged in.
10. «Выйти» logs out of NAX5; it does not power off the PS5.
EOF

cat > "$USER_DIR/BUILD-INFO.txt" <<EOF
NAX5 Alpha 0.5 portable Windows x64
release: ${RELEASE_TAG}
profile: product
repo: nax5-client
branch: ${BRANCH}
commit: ${SHA}
describe: ${DESCRIBE}
built: ${BUILT}
configuration: Release
chiaki.exe bytes: ${EXE_SIZE}
default API: https://cloudgta6.com
launch: chiaki.exe
unit tests: nax5-auth-unit, nax5-session-unit, nax5-connection-unit passed
features: login, reserve, auto Remote Play, process log, client-reports, session hardening
EOF

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
for candidate in \
  "/c/Program Files (x86)/Inno Setup 6/ISCC.exe" \
  "/c/Program Files/Inno Setup 6/ISCC.exe" \
  "/c/Users/${USERNAME}/AppData/Local/Programs/Inno Setup 6/ISCC.exe"
do
  if [[ -f "$candidate" ]]; then
    ISCC="$candidate"
    break
  fi
done

INSTALLER="$OUT/NAX5-Alpha-0.5-Windows-x64-setup.exe"
if [[ -n "$ISCC" ]]; then
  rm -f "$INSTALLER"
  "$ISCC" \
    "/DMyAppPath=$(cygpath -w "$USER_DIR")" \
    "/DMyOutputDir=$(cygpath -w "$OUT")" \
    "/DMyOutputBase=NAX5-Alpha-0.5-Windows-x64-setup" \
    "$(cygpath -w "$ROOT/scripts/nax5-windows-user.iss")"
  test -f "$INSTALLER"
  echo "INSTALLER=$INSTALLER"
fi

echo "DONE"
