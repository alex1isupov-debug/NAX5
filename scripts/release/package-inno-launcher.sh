#!/usr/bin/env bash
# Wrap a portable NAX5 tree in a Windows-native Inno Setup installer, then zip
# a single exe like chiaki-ng (no MSYS 7z SFX).
set -euo pipefail

export PATH="/mingw64/bin:/usr/bin:/bin:${PATH}"
export MSYSTEM=MINGW64

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <portable-user-dir> <output-dir>" >&2
  exit 1
fi

USER_DIR="$(cd "$1" && pwd)"
mkdir -p "$2"
OUT="$(cd "$2" && pwd)"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ISS="$ROOT/scripts/nax5-windows-user.iss"
INSTALLER="$OUT/NAX5-windows-installer.exe"
LAUNCHER_EXE="$OUT/NAX5.exe"
LAUNCHER_ZIP="$OUT/NAX5-windows.zip"
INSTALLER_ZIP="$OUT/NAX5-windows-installer.zip"

test -d "$USER_DIR"
test -f "$USER_DIR/chiaki.exe"
test -f "$ISS"

if ! compgen -G "$USER_DIR/avutil-*.dll" >/dev/null; then
  echo "Portable bundle is missing avutil-*.dll" >&2
  exit 1
fi
if ! compgen -G "$USER_DIR/avcodec-*.dll" >/dev/null; then
  echo "Portable bundle is missing avcodec-*.dll" >&2
  exit 1
fi
if ! compgen -G "$USER_DIR/avformat-*.dll" >/dev/null; then
  echo "Portable bundle is missing avformat-*.dll" >&2
  exit 1
fi
if ! compgen -G "$USER_DIR/swresample-*.dll" >/dev/null; then
  echo "Portable bundle is missing swresample-*.dll" >&2
  exit 1
fi

find_iscc() {
  if [[ -n "${ISCC:-}" && -f "$ISCC" ]]; then
    printf '%s\n' "$ISCC"
    return 0
  fi

  local candidates=()
  candidates+=(
    "/c/Program Files (x86)/Inno Setup 6/ISCC.exe"
    "/c/Program Files/Inno Setup 6/ISCC.exe"
  )
  if [[ -n "${LOCALAPPDATA:-}" ]]; then
    candidates+=("$(cygpath -u "$LOCALAPPDATA")/Programs/Inno Setup 6/ISCC.exe")
  fi
  if [[ -n "${USERNAME:-}" ]]; then
    candidates+=("/c/Users/${USERNAME}/AppData/Local/Programs/Inno Setup 6/ISCC.exe")
  fi
  local extra
  shopt -s nullglob
  for extra in /c/Users/*/AppData/Local/Programs/"Inno Setup 6"/ISCC.exe; do
    candidates+=("$extra")
  done
  shopt -u nullglob

  local candidate
  for candidate in "${candidates[@]}"; do
    if [[ -f "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

ISCC_BIN="$(find_iscc)" || {
  echo "Inno Setup 6 ISCC.exe is required to build NAX5-windows-installer.exe" >&2
  echo "Install from https://jrsoftware.org/isinfo.php and re-run this script." >&2
  exit 1
}

echo "ISCC=$ISCC_BIN"

resolve_powershell() {
  local candidate
  for candidate in \
    "${POWERSHELL:-}" \
    /c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe \
    /c/Windows/SysWOW64/WindowsPowerShell/v1.0/powershell.exe \
    powershell.exe
  do
    [[ -n "$candidate" ]] || continue
    if command -v "$candidate" >/dev/null 2>&1; then
      printf '%s\n' "$candidate"
      return 0
    fi
    if [[ -f "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

rm -f "$INSTALLER" "$LAUNCHER_EXE" "$LAUNCHER_ZIP" "$INSTALLER_ZIP"
mkdir -p "$OUT"

export MSYS2_ARG_CONV_EXCL='*'
"$ISCC_BIN" \
  "/DMyAppPath=$(cygpath -w "$USER_DIR")" \
  "/DMyOutputDir=$(cygpath -w "$OUT")" \
  "/DMyOutputBase=NAX5-windows-installer" \
  "$(cygpath -w "$ISS")"
test -f "$INSTALLER"

if command -v objdump >/dev/null; then
  if objdump -p "$INSTALLER" 2>/dev/null | grep -qi 'msys-'; then
    echo "error: installer imports MSYS runtime DLLs; refusing to ship" >&2
    exit 1
  fi
fi

cp -f "$INSTALLER" "$LAUNCHER_EXE"

POWERSHELL_BIN="$(resolve_powershell || true)"
if [[ -n "${POWERSHELL_BIN:-}" ]]; then
  INSTALLER_WIN="$(cygpath -w "$INSTALLER")"
  DESC="$(
    "$POWERSHELL_BIN" -NoProfile -Command "\$info=[System.Diagnostics.FileVersionInfo]::GetVersionInfo('${INSTALLER_WIN}'); \$info.FileDescription.Trim()" \
      | tr -d '\r'
  )"
  if [[ "$DESC" != "NAX5 Setup" ]]; then
    echo "Unexpected installer FileDescription: [$DESC]" >&2
    exit 1
  fi
fi

(
  cd "$OUT"
  zip -9 "$(basename "$LAUNCHER_ZIP")" "$(basename "$LAUNCHER_EXE")"
  zip -9 "$(basename "$INSTALLER_ZIP")" "$(basename "$INSTALLER")"
)

python3 - "$(cygpath -w "$LAUNCHER_ZIP")" "$(cygpath -w "$INSTALLER_ZIP")" <<'PY'
import sys
import zipfile

def check(path, expected):
    with zipfile.ZipFile(path) as zf:
        bad = zf.testzip()
        if bad:
            raise SystemExit(f'{path} failed CRC for {bad}')
        names = zf.namelist()
        if names != [expected]:
            raise SystemExit(f'{path} must contain only {expected}, got {names}')
    print(f'ZIP_OK {path} {expected}')

check(sys.argv[1], 'NAX5.exe')
check(sys.argv[2], 'NAX5-windows-installer.exe')
PY

if [[ -n "${POWERSHELL_BIN:-}" && "${NAX5_SKIP_LAUNCH_SMOKE:-0}" != "1" ]]; then
  "$POWERSHELL_BIN" -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$ROOT/scripts/release/verify-clean-windows-launch.ps1")" \
    -BundleDir "$(cygpath -w "$USER_DIR")" \
    -InstallerExe "$(cygpath -w "$INSTALLER")"
else
  echo "CLEAN_LAUNCH_NOT_TESTED: launch smoke skipped; do not mark this artifact release-verified"
fi

INSTALLER_SIZE="$(wc -c < "$INSTALLER" | tr -d '[:space:]')"
LAUNCHER_ZIP_SHA256="$(sha256sum "$LAUNCHER_ZIP" | awk '{print $1}')"
INSTALLER_ZIP_SHA256="$(sha256sum "$INSTALLER_ZIP" | awk '{print $1}')"

echo "INSTALLER=$INSTALLER"
echo "INSTALLER_BYTES=$INSTALLER_SIZE"
echo "LAUNCHER_ZIP=$LAUNCHER_ZIP"
echo "LAUNCHER_ZIP_SHA256=$LAUNCHER_ZIP_SHA256"
echo "INSTALLER_ZIP=$INSTALLER_ZIP"
echo "INSTALLER_ZIP_SHA256=$INSTALLER_ZIP_SHA256"
ls -l "$INSTALLER" "$LAUNCHER_EXE" "$LAUNCHER_ZIP" "$INSTALLER_ZIP"
