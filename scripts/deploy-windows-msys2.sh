#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 5 ]]; then
    echo "usage: $0 <output-dir> <exe-path> <tool-dir> <msys-prefix> <qml-dir>" >&2
    exit 1
fi

output_dir="$1"
exe_path="$2"
tool_dir="$3"
msys_prefix="$4"
qml_dir="$5"

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source_root="$(cd "$script_dir/.." && pwd)"
if [[ "$qml_dir" != /* ]]; then
    qml_dir="$source_root/$qml_dir"
fi

mkdir -p "$output_dir"
cp "$exe_path" "$output_dir/"

export PATH="${tool_dir}:${msys_prefix}/share/qt6/bin:${PATH}"
export QT_PLUGIN_PATH="${msys_prefix}/share/qt6/plugins"
export QML2_IMPORT_PATH="${msys_prefix}/share/qt6/qml"

ldd_timeout="${LDD_TIMEOUT:-10}"
ldd_timeout_cmd=()
if command -v timeout >/dev/null; then
    ldd_timeout_cmd=(timeout --kill-after=5s "${ldd_timeout}s")
else
    echo "warning: timeout(1) not found, ldd might hang" >&2
fi

declare -A queued_paths=()
declare -A scanned_paths=()

queue=("$output_dir/$(basename "$exe_path")")
queued_paths["$queue"]=1

extract_dependencies() {
    local binary="$1"

    local ldd_output
    local ldd_status=0
    if [[ ${#ldd_timeout_cmd[@]} -gt 0 ]]; then
        set +e
        ldd_output="$("${ldd_timeout_cmd[@]}" ldd "$binary" 2>&1)"
        ldd_status=$?
        set -e
        if [[ $ldd_status -eq 124 ]]; then
            echo "ldd timed out for $binary" >&2
        elif [[ $ldd_status -ne 0 ]]; then
            echo "ldd exited with status $ldd_status for $binary" >&2
        fi
    else
        ldd_output="$(ldd "$binary" 2>&1)" || true
    fi

    printf '%s\n' "$ldd_output" | awk '
        /=>/ && $(NF-1) != "not" { print $(NF-1) }
        /^\// { print $1 }
    ' | grep -iv "system32" | grep -iv "windows" || true
}

enqueue_dependency() {
    local dependency="$1"
    local file_name

    [[ -n "$dependency" ]] || return 0
    [[ -f "$dependency" ]] || return 0

    file_name="${dependency##*/}"
    if [[ ! -e "$output_dir/$file_name" ]]; then
        echo "Copied $dependency"
        cp "$dependency" "$output_dir/"
    fi

    if [[ -z "${queued_paths["$dependency"]+x}" ]]; then
        queue+=("$dependency")
        queued_paths["$dependency"]=1
    fi
}

copy_msys_bin() {
    local pattern="$1"
    local dll

    shopt -s nullglob
    for dll in "$msys_prefix"/bin/$pattern; do
        enqueue_dependency "$dll"
    done
    shopt -u nullglob
}

require_msys_bin() {
    local pattern="$1"
    local matches=()

    shopt -s nullglob
    matches=("$msys_prefix"/bin/$pattern)
    shopt -u nullglob

    if [[ ${#matches[@]} -eq 0 ]]; then
        echo "error: required runtime missing in $msys_prefix/bin/$pattern" >&2
        exit 1
    fi
}

while [[ ${#queue[@]} -gt 0 ]]; do
    current="${queue[0]}"
    queue=("${queue[@]:1}")

    if [[ -n "${scanned_paths["$current"]+x}" ]]; then
        continue
    fi
    scanned_paths["$current"]=1

    while IFS= read -r dependency; do
        enqueue_dependency "$dependency"
    done < <(extract_dependencies "$current")
done

# chiaki-ng explicitly bundles FFmpeg/SDL/libplacebo DLLs; ldd alone misses
# runtime-loaded codecs (avutil-59.dll, avcodec-*.dll, etc.).
require_msys_bin 'avutil-*.dll'
require_msys_bin 'avcodec-*.dll'
require_msys_bin 'avformat-*.dll'
require_msys_bin 'swresample-*.dll'
copy_msys_bin 'avutil-*.dll'
copy_msys_bin 'avcodec-*.dll'
copy_msys_bin 'avformat-*.dll'
copy_msys_bin 'swresample-*.dll'
copy_msys_bin 'swscale-*.dll'
copy_msys_bin 'SDL2.dll'
copy_msys_bin 'SDL3.dll'
copy_msys_bin 'libplacebo-*.dll'
copy_msys_bin 'shaderc_shared.dll'
copy_msys_bin 'libshaderc_shared.dll'
copy_msys_bin 'spirv-cross-c-shared.dll'
copy_msys_bin 'libspirv-cross-c-shared.dll'
copy_msys_bin 'libssl-*.dll'
copy_msys_bin 'libcrypto-*.dll'

windeployqt6.exe --no-translations --qmldir="$qml_dir" "$output_dir/$(basename "$exe_path")"

# Qt plugins are added after the executable dependency walk. Include their
# dependencies, and any transitive dependencies, in the portable directory.
while IFS= read -r -d '' plugin; do
    queue+=("$plugin")
done < <(find "$output_dir" -type f -iname '*.dll' -print0)

while [[ ${#queue[@]} -gt 0 ]]; do
    current="${queue[0]}"
    queue=("${queue[@]:1}")

    if [[ -n "${scanned_paths["$current"]+x}" ]]; then
        continue
    fi
    scanned_paths["$current"]=1

    while IFS= read -r dependency; do
        enqueue_dependency "$dependency"
    done < <(extract_dependencies "$current")
done

# Remove system-provided DLLs that Windows already supplies and
# should not be bundled with the MSYS2 build (e.g. D3D compiler). Use
# case-insensitive globbing to catch variants like `D3Dcompiler_47`.
(
    shopt -s nocaseglob
    rm -f "$output_dir"/d3dcompiler*.dll
)

# Include the source license and downstream attribution notices in the
# portable NAX5 distribution.
cp "$source_root/COPYING" "$output_dir/COPYING"
cp -a "$source_root/LICENSES" "$output_dir/LICENSES"
cp "$source_root/UPSTREAM.md" "$output_dir/UPSTREAM.md"
cp "$source_root/THIRD-PARTY-NOTICES.md" "$output_dir/THIRD-PARTY-NOTICES.md"

shopt -s nullglob
for pattern in avutil-*.dll avcodec-*.dll avformat-*.dll swresample-*.dll; do
    if ! compgen -G "$output_dir/$pattern" > /dev/null; then
        echo "error: portable bundle is missing $pattern" >&2
        exit 1
    fi
done
shopt -u nullglob

if ! command -v objdump >/dev/null; then
    echo "error: objdump is required to verify portable DLL imports" >&2
    exit 1
fi

declare -A system32_dlls=()
if [[ -d /c/Windows/System32 ]]; then
    while IFS= read -r system_dll; do
        system32_dlls["${system_dll,,}"]=1
    done < <(ls /c/Windows/System32)
fi

bundle_has_dll() {
    local needle="$1"
    local pe_dir="$2"
    local match

    [[ -f "$pe_dir/$needle" || -f "$output_dir/$needle" ]] && return 0
    match="$(find "$output_dir" -iname "$needle" -print -quit)"
    [[ -n "$match" ]]
}

collect_unresolved_imports() {
    local pe pe_dir dll base lower
    while IFS= read -r -d '' pe; do
        pe_dir="$(dirname "$pe")"
        while IFS= read -r dll; do
            [[ -n "$dll" ]] || continue
            base="${dll##*/}"
            lower="${base,,}"
            case "$lower" in
                api-ms-*.dll|ext-ms-*.dll) continue ;;
            esac
            if [[ -n "${system32_dlls[$lower]+x}" ]]; then
                continue
            fi
            if bundle_has_dll "$base" "$pe_dir"; then
                continue
            fi
            printf '%s\n' "$base"
        done < <(objdump -p "$pe" 2>/dev/null | awk '/DLL Name:/{print $3}' | sed 's/\r$//')
    done < <(find "$output_dir" \( -iname '*.dll' -o -iname '*.exe' \) -print0)
}

walk_queued_dependencies() {
    local current
    while [[ ${#queue[@]} -gt 0 ]]; do
        current="${queue[0]}"
        queue=("${queue[@]:1}")

        if [[ -n "${scanned_paths["$current"]+x}" ]]; then
            continue
        fi
        scanned_paths["$current"]=1

        while IFS= read -r dependency; do
            enqueue_dependency "$dependency"
        done < <(extract_dependencies "$current")
    done
}

resolve_objdump_imports() {
    local round dll fail copied
    local unresolved=()

    for round in 1 2 3 4 5 6; do
        mapfile -t unresolved < <(collect_unresolved_imports | sort -u)
        if [[ ${#unresolved[@]} -eq 0 || -z "${unresolved[0]:-}" ]]; then
            echo "objdump import check passed"
            return 0
        fi

        fail=0
        copied=0
        for dll in "${unresolved[@]}"; do
            if [[ -f "$msys_prefix/bin/$dll" ]]; then
                echo "objdump: copying missing import $dll (round $round)"
                enqueue_dependency "$msys_prefix/bin/$dll"
                copied=1
            else
                echo "error: unresolved import $dll (not in bundle or $msys_prefix/bin)" >&2
                fail=1
            fi
        done
        if [[ $fail -ne 0 ]]; then
            return 1
        fi
        if [[ $copied -eq 1 ]]; then
            walk_queued_dependencies
        fi
    done

    echo "error: objdump imports still unresolved after retries:" >&2
    printf '  %s\n' "${unresolved[@]}" >&2
    return 1
}

resolve_objdump_imports
