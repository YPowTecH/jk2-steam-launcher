#!/usr/bin/env bash
# Lint jk2-steam-launcher with clang-format (formatting) and clang-tidy
# (static analysis, configured in .clang-tidy). MSVC's /W4 /WX /permissive-
# and /analyze run as part of the normal CMake build.
#
# Usage: tools/lint.sh [--fix]
#   --fix  reformat the sources in place instead of only checking
#
# Needs LLVM (clang-format, clang-tidy) on PATH or in LLVM_BIN, and Visual
# Studio installed (clang-tidy uses its Windows SDK headers).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LLVM_BIN="${LLVM_BIN:-/c/Program Files/LLVM/bin}"
if [ -x "$LLVM_BIN/clang-tidy.exe" ]; then
	PATH="$LLVM_BIN:$PATH"
fi

cd "$ROOT"
SOURCES=(src/*.cpp)

# Same configuration as the CMake build: 32-bit MSVC target, C++17, Unicode.
# _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH: newer MSVC STL headers insist on
# a newer Clang than the installed clang-tidy; the STL's documented opt-out.
COMPILE_FLAGS=(
	--target=i686-pc-windows-msvc -std=c++17 -fms-extensions -fms-compatibility
	-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX
	-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH
	-Wall -Wextra
)

if [ "${1:-}" = "--fix" ]; then
	clang-format -i "${SOURCES[@]}"
	echo "clang-format: reformatted ${SOURCES[*]}"
else
	clang-format --dry-run --Werror "${SOURCES[@]}"
	echo "clang-format: ok"
fi

clang-tidy --quiet "${SOURCES[@]}" -- "${COMPILE_FLAGS[@]}"
echo "clang-tidy: ok"
