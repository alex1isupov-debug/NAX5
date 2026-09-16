#!/usr/bin/env bash
# One-time MSYS2 MINGW64 dependency install for NAX5 Windows builds.
set -euo pipefail

export MSYSTEM=MINGW64
export PATH="/mingw64/bin:/usr/bin:/bin:${PATH}"

if [[ "${MSYSTEM:-}" != "MINGW64" && "$(uname -o 2>/dev/null || true)" != "Msys" ]]; then
  echo "Run this script from MSYS2 MINGW64." >&2
  exit 1
fi

if [[ "${SKIP_PACMAN:-0}" != "1" ]]; then
  pacman -Syu --noconfirm
  pacman -S --needed --noconfirm \
  git make unzip zip \
  mingw-w64-x86_64-toolchain \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-pkgconf \
  mingw-w64-x86_64-cc \
  mingw-w64-x86_64-curl \
  mingw-w64-x86_64-openssl \
  mingw-w64-x86_64-opus \
  mingw-w64-x86_64-speexdsp \
  mingw-w64-x86_64-fftw \
  mingw-w64-x86_64-hidapi \
  mingw-w64-x86_64-json-c \
  mingw-w64-x86_64-libevent \
  mingw-w64-x86_64-miniupnpc \
  mingw-w64-x86_64-protobuf \
  mingw-w64-x86_64-python \
  mingw-w64-x86_64-python-protobuf \
  mingw-w64-x86_64-python-pip \
  mingw-w64-x86_64-python-psutil \
  mingw-w64-x86_64-python-glad \
  mingw-w64-x86_64-python-jinja \
  mingw-w64-x86_64-qt6-base \
  mingw-w64-x86_64-qt6-declarative \
  mingw-w64-x86_64-qt6-svg \
  mingw-w64-x86_64-vulkan \
  mingw-w64-x86_64-vulkan-headers \
  mingw-w64-x86_64-shaderc \
  mingw-w64-x86_64-spirv-cross \
  mingw-w64-x86_64-meson \
  mingw-w64-x86_64-nasm \
  mingw-w64-x86_64-fast_float \
  mingw-w64-x86_64-lcms2 \
  mingw-w64-x86_64-libdovi
fi

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

if [[ ! -f /mingw64/bin/avcodec-61.dll ]]; then
  curl -LO https://github.com/streetpea/FFmpeg-Builds/releases/download/latest/ffmpeg-n7.1-latest-win64-gpl-shared-7.1.zip
  unzip -o ffmpeg-n7.1-latest-win64-gpl-shared-7.1.zip
  cp -a ffmpeg-n7.1-latest-win64-gpl-shared-7.1/bin/. /mingw64/bin/
  cp -a ffmpeg-n7.1-latest-win64-gpl-shared-7.1/include/. /mingw64/include/
  cp -a ffmpeg-n7.1-latest-win64-gpl-shared-7.1/lib/. /mingw64/lib/
fi

if [[ ! -f /mingw64/lib/libplacebo.dll ]]; then
  LIBPLACEBO_VERSION="${LIBPLACEBO_VERSION:-v7.360.1}"
  scripts/build-libplacebo-windows.sh
fi

if [[ ! -f /mingw64/lib/pkgconfig/sdl2.pc ]] || grep -q sdl2-compat /mingw64/lib/pkgconfig/sdl2.pc 2>/dev/null; then
  INSTALL_PREFIX=/mingw64 scripts/build-sdl2-compat.sh .
  cp /mingw64/lib/pkgconfig/sdl2-compat.pc /mingw64/lib/pkgconfig/sdl2.pc
fi

/mingw64/bin/python3 -c 'import google.protobuf'
echo "MSYS2 build environment is ready."
